#include "data.hh"

// Hack to get the DLL instance without a DllMain()
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#pragma warning(disable: 4047)
HINSTANCE hInstance = (HINSTANCE)&__ImageBase;
#pragma warning(default: 4047)

std::unordered_map<napi_env, EnvData> env_datas;

template <typename K, typename V>
V* try_get(std::unordered_map<K, V>& items, K key)
{
    if (auto it = items.find(key); it != items.end())
    {
        return &it->second;
    }

    return nullptr;
}

EnvData* get_env_data(napi_env env)
{
    return try_get(env_datas, env);
}

IconData* get_icon_data(napi_env env, int32_t icon_id)
{
    if (auto env_data = get_env_data(env); env_data)
    {
        return try_get(env_data->icons, icon_id);
    }

    return nullptr;
}

IconData* get_icon_data_by_menu(napi_env env, HMENU menu, int32_t item_id)
{
    if (auto env_data = get_env_data(env); env_data)
    {
        for (auto& entry : env_data->icons)
        {
            if (entry.second.menu == menu)
            {
                return &entry.second;
            }
        }
    }

    return nullptr;
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
                        icon_data && icon_data->menu)
                    {
                        // Required to hide menu after select or click-outside.
                        SetForegroundWindow(hwnd);
                        auto item_id = (int32_t)TrackPopupMenuEx(
                            icon_data->menu,
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

EnvData* get_or_create_env_data(napi_env env)
{
    if (auto data = get_env_data(env); data)
    {
        return data;
    }

    auto data = &env_datas[env];

    uv_loop_s* loop = nullptr;
    napi_get_uv_event_loop(env, &loop);
    uv_idle_init(loop, &data->message_pump_idle);

    if (!windowClassId)
    {
        WNDCLASSW wc = {};
        wc.lpszClassName = L"Tray Message Window";
        wc.lpfnWndProc = messageWndProc;
        wc.hInstance = hInstance;
        windowClassId = RegisterClassW(&wc);
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

    data->env = env;
    data->msg_hwnd = hwnd;

    napi_add_env_cleanup_hook(
        env,
        [](void* args)
        {
            // This will also fire all the required destructors.
            env_datas.erase((napi_env) args);
        },
        env);

    return data;
}