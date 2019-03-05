#include "data.hh"
#include "menu-object.hh"

#include <map>
#include <mutex>

// Hack to get the DLL instance without a DllMain()
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#pragma warning(disable: 4047)
HINSTANCE hInstance = (HINSTANCE)&__ImageBase;
#pragma warning(default: 4047)

// If you have multiple environments in one process,
// then you have multiple threads.
static std::mutex env_datas_mutex;
// Must be map<> for the invalidation guarantees
static std::map<napi_env, EnvData> env_datas;

EnvData* get_env_data(napi_env env)
{
    std::lock_guard lock{env_datas_mutex};
    if (auto it = env_datas.find(env); it != env_datas.end())
    {
        return &it->second;
    }

    return nullptr;
}

IconData* get_icon_data(napi_env env, int32_t icon_id)
{
    if (auto env_data = get_env_data(env); env_data)
    {
        return env_data->get_icon_data(icon_id);
    }

    return nullptr;
}

IconData* get_icon_data_by_menu(napi_env env, HMENU menu, int32_t item_id)
{
    if (auto env_data = get_env_data(env); env_data)
    {
        for (auto& entry : env_data->icons)
        {
            if (entry.second.context_menu_ref.wrapped->menu == menu)
            {
                return &entry.second;
            }
        }
    }

    return nullptr;
}

static void message_pump_idle_cb(uv_idle_t* idle)
{
    auto data = (EnvData*) idle->data;

    // Poll for messages without blocking either the node event loop for too long (10ms)
    // or burning an entire CPU. Would be nicer to use a GetMessage() loop on another
    // thread, but it seems a bit over complicated since this mostly works fine
    // (0% CPU on my machine).
    // Be aware this does still prevent the CPU from getting to sleep mode, so it's not perfect.
    // https://stackoverflow.com/a/10866466/20135
    if (MsgWaitForMultipleObjects(0, nullptr, false, 10, QS_ALLINPUT) == WAIT_OBJECT_0)
    {
        MSG msg = {};
        while (PeekMessage(&msg, data->msg_hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

IconData* EnvData::get_icon_data(int32_t icon_id)
{
    if (auto it = icons.find(icon_id); it != icons.end())
    {
        return &it->second;
    }

    return nullptr;
}

void EnvData::start_message_pump()
{
    uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
}

void EnvData::stop_message_pump()
{
    uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
}

LRESULT messageWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            auto pcs = (CREATESTRUCTW*) lparam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) pcs->lpCreateParams);
            break;
        }
        case WM_TRAYICON_CALLBACK:
        {
            switch (LOWORD(lparam))
            {
                // case WM_LBUTTONUP:
                // case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                // case NIN_POPUPOPEN:
                case NIN_SELECT:
                // case NIN_KEYSELECT:
                    auto env = (napi_env) (void*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
                    auto icon_id = HIWORD(lparam);
                    if (auto icon_data = get_icon_data(env, icon_id);
                        icon_data && icon_data->context_menu_ref.wrapped)
                    {
                        // Required to hide menu after select or click-outside.
                        SetForegroundWindow(hwnd);
                        HMENU menu = icon_data->context_menu_ref.wrapped->menu;

                        auto item_id = (int32_t)TrackPopupMenuEx(
                            menu,
                            GetSystemMetrics(SM_MENUDROPALIGNMENT) | TPM_RETURNCMD | TPM_NONOTIFY,
                            // needs sign-extension for multiple monitors
                            (int16_t)LOWORD(wparam),
                            (int16_t)HIWORD(wparam),
                            hwnd,
                            nullptr);
                        if (item_id && icon_data->select_callback)
                        {
                            icon_data->select_callback(item_id);
                        }
                    }
                    break;
            }
            break;
        }
        // case WM_MENUSELECT:
        // {
        //     auto menu = (HMENU) lparam;
        //     auto flags = HIWORD(wparam);
        //     if (flags & MF_POPUP)
        //     {
        //         auto env = (napi_env) (void*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        //         int32_t item_id = LOWORD(wparam);
        //         if (auto icon_data = get_icon_data_by_menu(env, menu, item_id);
        //             icon_data && icon_data->select_callback)
        //         {
        //         }
        //     }
        // }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

ATOM windowClassId = 0;

std::tuple<napi_status, EnvData*> create_env_data(napi_env env)
{
    // Lock for the whole method just so we know we'll get
    // a consistent EnvData.
    std::lock_guard lock{env_datas_mutex};
    auto data = &env_datas[env];
    data->env = env;

    if (auto status = napi_add_env_cleanup_hook(
            env,
            [](void* args)
            {
                std::lock_guard lock{env_datas_mutex};
                // This will also fire all the required destructors.
                env_datas.erase((napi_env) args);
            },
            env);
        status != napi_ok)
    {
        return {status, nullptr};
    }

    uv_loop_s* loop = nullptr;
    if (auto status = napi_get_uv_event_loop(env, &loop);
        status != napi_ok)
    {
        return {status, nullptr};
    }
    uv_idle_init(loop, &data->message_pump_idle);

    if (!windowClassId)
    {
        WNDCLASSW wc = {};
        wc.lpszClassName = L"Tray Message Window";
        wc.lpfnWndProc = messageWndProc;
        wc.hInstance = hInstance;
        windowClassId = RegisterClassW(&wc);
        if (!windowClassId)
        {
            napi_throw_win32_error(env, "RegisterClassW");
            return {napi_pending_exception, data};
        }
    }

    HWND hwnd = CreateWindowW(
        (LPWSTR)windowClassId,
        L"Tray Message Window",
        0, // style
        0, // x
        0, // y
        0, // width
        0, // height
        HWND_MESSAGE, // parent
        nullptr, // menu
        hInstance, // instance
        env); // param
    if (!hwnd)
    {
        napi_throw_win32_error(env, "CreateWindowW");
        return {napi_pending_exception, data};
    }

    data->msg_hwnd = hwnd;

    return {napi_ok, data};
}