#include "data.hh"
#include "notify-icon-object.hh"

#include <map>
#include <mutex>

#include <shellapi.h>

// Hack to get the DLL instance without a DllMain()
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HINSTANCE get_image_instance() {
  return reinterpret_cast<HINSTANCE>(&__ImageBase);
}

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

napi_status EnvData::add_icon(int32_t id, napi_value value,
                              NotifyIconObject* object) {
  if (icons.empty()) {
    uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
  }
  napi_ref ref;
  NAPI_RETURN_IF_NOT_OK(napi_create_reference(env, value, 1, &ref));
  if (icons
          .insert({id,
                   {ref, object->notify_icon.id.callback_id,
                    object->notify_icon.id.guid}})
          .second == false) {
    NAPI_RETURN_IF_NOT_OK(napi_delete_reference(env, ref));
  }
  return napi_ok;
}

bool EnvData::remove_icon(int32_t id) {
  if (auto it = icons.find(id); it == icons.end()) {
    return false;
  } else {
    icons.erase(it);
    if (icons.empty()) {
      uv_idle_stop(&message_pump_idle);
    }
    return true;
  }
}

static napi_status notify_select(EnvData* env_data, int32_t icon_id,
                                 bool right_button, int32_t mouse_x,
                                 int32_t mouse_y) {
  if (auto it = env_data->icons.find(icon_id); it != env_data->icons.end()) {
    auto env = env_data->env;
    napi_value value;
    NAPI_RETURN_IF_NOT_OK(
        napi_get_reference_value(env, it->second.ref, &value));
    NotifyIconObject* object;
    NAPI_RETURN_IF_NOT_OK(napi_get_value(env, value, &object));
    NAPI_RETURN_IF_NOT_OK(
        object->select(env, value, right_button, mouse_x, mouse_y));
  }
  return napi_ok;
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
        case NIN_KEYSELECT:
        case WM_CONTEXTMENU:
          // Required to hide menu after select or click-outside, if you are
          // going to show a menu. Firing now rather than later as we can only
          // take foreground while responding to user interaction.
          SetForegroundWindow(hwnd);

          auto env = (napi_env)(void*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
          auto icon_id = HIWORD(lParam);
          auto right_button = LOWORD(lParam) == WM_CONTEXTMENU;
          // needs sign-extension for multiple monitors
          // Equivalent to GET_X_LPARAM
          auto mouse_x = (int16_t)LOWORD(wParam);
          auto mouse_y = (int16_t)HIWORD(wParam);

          if (auto env_data = get_env_data(env); env_data) {
            NapiHandleScope scope;
            // if this fails, we can't even throw an error!
            NAPI_FATAL_IF_NOT_OK(scope.open(env));
            if (notify_select(env_data, icon_id, right_button, mouse_x,
                              mouse_y) != napi_ok) {
              napi_throw_async_last_error(env);
            }
          }
      }
      break;
    }
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

  auto hInstance = get_image_instance();

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
  if (!SetThreadDpiAwarenessContext) {
    SetThreadDpiAwarenessContext = [](DPI_AWARENESS_CONTEXT context) {
      return context;
    };
  }

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

EnvData::~EnvData() {
  for (auto& pair : icons) {
    delete_notify_icon({msg_hwnd, pair.second.id, pair.second.guid});
  }
}
