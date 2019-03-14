#include "data.hh"
#include "menu-object.hh"

#include <map>
#include <mutex>

#include <shellapi.h>

// Hack to get the DLL instance without a DllMain()
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#pragma warning(disable : 4047)
HINSTANCE hInstance = (HINSTANCE)&__ImageBase;
#pragma warning(default : 4047)

// If you have multiple environments in one process,
// then you have multiple threads.
static std::mutex env_datas_mutex;
// Must be map<> for the invalidation guarantees
static std::map<napi_env, EnvData> env_datas;

EnvData* get_env_data(napi_env env) {
  std::lock_guard lock{env_datas_mutex};
  if (auto it = env_datas.find(env); it != env_datas.end()) {
    return &it->second;
  }

  return nullptr;
}

IconData* get_icon_data(napi_env env, int32_t icon_id) {
  if (auto env_data = get_env_data(env); env_data) {
    return env_data->get_icon_data(icon_id);
  }

  return nullptr;
}

static void message_pump_idle_cb(uv_idle_t* idle) {
  auto data = (EnvData*)idle->data;

  // Poll for messages without blocking either the node event loop for too long
  // (10ms) or burning an entire CPU. Would be nicer to use a GetMessage() loop
  // on another thread, but it seems a bit over complicated since this mostly
  // works fine (0% CPU on my machine). Be aware this does still prevent the CPU
  // from getting to sleep mode, so it's not perfect.
  // https://stackoverflow.com/a/10866466/20135
  if (MsgWaitForMultipleObjects(0, nullptr, false, 10, QS_ALLINPUT) ==
      WAIT_OBJECT_0) {
    MSG msg = {};
    while (PeekMessage(&msg, data->msg_hwnd, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

IconData* EnvData::get_icon_data(int32_t icon_id) {
  if (auto it = icons.find(icon_id); it != icons.end()) {
    return &it->second;
  }

  return nullptr;
}

void EnvData::start_message_pump() {
  uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
}

void EnvData::stop_message_pump() {
  uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
}

LRESULT messageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      auto pcs = (CREATESTRUCTW*)lParam;
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
      break;
    }
    case WM_TRAYICON_CALLBACK: {
      switch (LOWORD(lParam)) {
        case NIN_SELECT:
        case WM_CONTEXTMENU:
          auto env = (napi_env)(void*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
          auto icon_id = HIWORD(lParam);
          auto right_button = LOWORD(lParam) == WM_CONTEXTMENU;
          // needs sign-extension for multiple monitors
          // Equivalent to GET_X_LPARAM
          auto mouse_x = (int16_t)LOWORD(wParam);
          auto mouse_y = (int16_t)HIWORD(wParam);

          if (auto icon_data = get_icon_data(env, icon_id); !icon_data)
            return napi_ok;
          else {
            NapiHandleScope scope;
            scope.open(env);

            // Required to hide menu after select or click-outside, if you are
            // going to show a menu. Firing now rather than later as we can only
            // take foreground while responding to user interaction.
            SetForegroundWindow(hwnd);

            napi_value event;
            napi_create_object(env, &event,
                               {
                                   {"iconId", icon_id},
                                   {"rightButton", right_button},
                                   {"mouseX", mouse_x},
                                   {"mouseY", mouse_y},
                               });

            icon_data->select_callback(nullptr, event);
          }
          break;
      }
      break;
    }
      // case WM_MENUSELECT:
      // {
      //     auto menu = (HMENU) lParam;
      //     auto flags = HIWORD(wParam);
      //     if (flags & MF_POPUP)
      //     {
      //         auto env = (napi_env) (void*) GetWindowLongPtrW(hwnd,
      //         GWLP_USERDATA); int32_t item_id = LOWORD(wParam); if (auto
      //         icon_data = get_icon_data_by_menu(env, menu, item_id);
      //             icon_data && icon_data->select_callback)
      //         {
      //         }
      //     }
      // }
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

ATOM windowClassId = 0;

std::tuple<napi_status, EnvData*> create_env_data(napi_env env) {
  // Lock for the whole method just so we know we'll get
  // a consistent EnvData.
  std::lock_guard lock{env_datas_mutex};
  auto data = &env_datas[env];
  data->env = env;

  if (auto status =
          napi_add_env_cleanup_hook(env,
                                    [](void* args) {
                                      std::lock_guard lock{env_datas_mutex};
                                      // This will also fire all the required
                                      // destructors.
                                      env_datas.erase((napi_env)args);
                                    },
                                    env);
      status != napi_ok) {
    return {status, nullptr};
  }

  uv_loop_s* loop = nullptr;
  if (auto status = napi_get_uv_event_loop(env, &loop); status != napi_ok) {
    return {status, nullptr};
  }
  uv_idle_init(loop, &data->message_pump_idle);

  if (!windowClassId) {
    WNDCLASSW wc = {};
    wc.lpszClassName = L"Tray Message Window";
    wc.lpfnWndProc = messageWndProc;
    wc.hInstance = hInstance;
    windowClassId = RegisterClassW(&wc);
    if (!windowClassId) {
      napi_throw_win32_error(env, "RegisterClassW");
      return {napi_pending_exception, data};
    }
  }

  // Please give me the real screen positions of clicks, but don't break other
  // addons.
  auto SetThreadDpiAwarenessContext =
      (DPI_AWARENESS_CONTEXT(*)(DPI_AWARENESS_CONTEXT))GetProcAddress(
          GetModuleHandle(L"user32"), "SetThreadDpiAwarenessContext");

  auto old_dpi_awareness =
      SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  HWND hwnd = CreateWindowW((LPWSTR)windowClassId, L"Tray Message Window",
                            0,             // style
                            0,             // x
                            0,             // y
                            0,             // width
                            0,             // height
                            HWND_MESSAGE,  // parent
                            nullptr,       // menu
                            hInstance,     // instance
                            env);          // param

  SetThreadDpiAwarenessContext(old_dpi_awareness);

  if (!hwnd) {
    napi_throw_win32_error(env, "CreateWindowW");
    return {napi_pending_exception, data};
  }

  data->msg_hwnd = hwnd;

  return {napi_ok, data};
}