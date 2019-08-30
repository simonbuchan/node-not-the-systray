#include "notify-icon-message-loop.hh"
#include "data.hh"
#include "unique.hh"

#include <future>
#include <variant>

#include <shellapi.h>

using WndHandle = Unique<HWND, DestroyWindow>;

// Hack to get the DLL instance without a DllMain()
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HINSTANCE get_image_instance() {
  return reinterpret_cast<HINSTANCE>(&__ImageBase);
}

constexpr auto WM_USER_QUIT = WM_USER;
constexpr auto WM_USER_CALL = WM_USER + 1;
constexpr auto WM_USER_NOTIFICATION_ICON = WM_USER + 2;

static ATOM windowClassId = 0;

struct MsgThreadError {
  const char* syscall;
  DWORD code;
};
using MsgThreadInitResult = std::variant<MsgThreadError, HWND>;

void msg_thread_proc(napi_env env,
                     std::promise<MsgThreadInitResult>& init_result);

NotifyIconMessageLoop::~NotifyIconMessageLoop() { quit(); }

UINT NotifyIconMessageLoop::notify_message() {
  return WM_USER_NOTIFICATION_ICON;
}

napi_status NotifyIconMessageLoop::init(EnvData* data) {
  std::promise<MsgThreadInitResult> msg_thread_init;
  auto msg_thread_init_promise = msg_thread_init.get_future();
  auto env = data->env;

  thread = std::thread(&msg_thread_proc, env, std::ref(msg_thread_init));
  auto init_result = msg_thread_init_promise.get();

  if (auto error = std::get_if<MsgThreadError>(&init_result); error) {
    NAPI_RETURN_IF_NOT_OK(
        napi_throw_win32_error(data->env, error->syscall, error->code));
    return napi_pending_exception;
  } else {
    hwnd = std::get<HWND>(init_result);
    return napi_ok;
  }
}

void NotifyIconMessageLoop::quit() {
  if (thread.joinable()) {
    PostMessage(hwnd, WM_USER_QUIT, 0, 0);
    thread.join();
  }
}

void NotifyIconMessageLoop::run_on_msg_thread_blocking_(
    unique_function<void()> body) {
  SendMessage(hwnd, WM_USER_CALL, 0, reinterpret_cast<LPARAM>(body.get()));
}

void NotifyIconMessageLoop::run_on_msg_thread_nonblocking_(
    unique_function<void()> body) {
  if (PostMessage(hwnd, WM_USER_CALL, 1,
                  reinterpret_cast<LPARAM>(body.get()))) {
    body.release();
  }
}

LRESULT messageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      auto pcs = (CREATESTRUCTW*)lParam;
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
      break;
    }
    case WM_USER_QUIT: {
      // https://devblogs.microsoft.com/oldnewthing/20090112-00/?p=19533
      ::PostQuitMessage(wParam);
      break;
    }
    case WM_USER_CALL: {
      auto body_ptr = reinterpret_cast<std::function<void()>*>(lParam);
      unique_function<void()> unique_body_ptr;
      if (wParam) {
        unique_body_ptr.reset(body_ptr);
      }
      (*body_ptr)();
      break;
    }
    case WM_USER_NOTIFICATION_ICON: {
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
          EnvData::NotifySelectArgs args;
          args.right_button = LOWORD(lParam) == WM_CONTEXTMENU;
          // needs sign-extension for multiple monitors
          // Equivalent to GET_X_LPARAM
          args.mouse_x = (int16_t)LOWORD(wParam);
          args.mouse_y = (int16_t)HIWORD(wParam);

          if (auto env_data = get_env_data(env); env_data) {
            env_data->notify_select(icon_id, args);
          }
      }
      break;
    }
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

void msg_thread_proc(napi_env env,
                     std::promise<MsgThreadInitResult>& init_result) {
  auto hInstance = get_image_instance();

  if (!windowClassId) {
    WNDCLASSW wc = {};
    wc.lpszClassName = L"Tray Message Window";
    wc.lpfnWndProc = messageWndProc;
    wc.hInstance = hInstance;
    windowClassId = RegisterClassW(&wc);
    if (!windowClassId) {
      init_result.set_value_at_thread_exit(
          MsgThreadError{"RegisterClassW", GetLastError()});
      return;
    }
  }

  // Please give me the real screen positions of clicks.
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

  WndHandle hwnd =
      CreateWindowW((LPWSTR)windowClassId, L"Tray Message Window", 0, 0, 0, 0,
                    0, HWND_MESSAGE, nullptr, hInstance, env);

  SetThreadDpiAwarenessContext(old_dpi_awareness);

  if (!hwnd) {
    init_result.set_value_at_thread_exit(
        MsgThreadError{"CreateWindowW", GetLastError()});
    return;
  }

  init_result.set_value(hwnd);

  MSG msg = {};
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
