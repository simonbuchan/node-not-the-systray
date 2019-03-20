#include "notify-icon.hh"

#include <shellapi.h>

template <size_t N>
wchar_t* copy(const std::wstring& s, wchar_t (&dest)[N]) {
  return std::copy(s.begin(), s.end(), dest);
}

template <size_t N>
wchar_t* copy(const std::optional<std::wstring>& maybe_s, wchar_t (&dest)[N]) {
  if (!maybe_s) {
    return dest;
  }
  return copy(maybe_s.value(), dest);
}

static NOTIFYICONDATAW make_data(const notify_icon_id& id) {
  NOTIFYICONDATAW data = {sizeof(data)};
  data.hWnd = id.callback_hwnd;
  data.uID = id.callback_id;
  if (id.guid) {
    data.uFlags = NIF_GUID;
    data.guidItem = id.guid.value();
  }
  return data;
}

static NOTIFYICONDATAW make_data(const notify_icon_id& id,
                                 const notify_icon_options& options) {
  auto data = make_data(id);

  if (options.hidden) {
    data.uFlags |= NIF_STATE;
    data.dwStateMask |= NIS_HIDDEN;
    if (options.hidden.value()) {
      data.dwState |= NIS_HIDDEN;
    }
  }

  if (options.icon) {
    data.uFlags |= NIF_ICON;
    data.hIcon = options.icon.value();
  }

  if (options.tooltip) {
    data.uFlags |= NIF_TIP | NIF_SHOWTIP;
    copy(options.tooltip.value(), data.szTip);
  }

  if (options.notification) {
    auto& notification = options.notification.value();
    data.uFlags |= NIF_INFO;
    if (notification.realtime.value_or(false)) data.uFlags |= NIF_REALTIME;
    if (!notification.sound.value_or(true)) data.dwInfoFlags |= NIIF_NOSOUND;
    if (notification.respect_quiet_time.value_or(true))
      data.dwInfoFlags |= NIIF_RESPECT_QUIET_TIME;

    if (notification.icon) {
      data.dwInfoFlags |= NIIF_USER;
      data.hBalloonIcon = notification.icon.value();
      if (notification.large_icon) {
        data.dwInfoFlags |= NIIF_LARGE_ICON;
      }
    }

    copy(notification.title, data.szInfoTitle);
    copy(notification.text, data.szInfo);
  }

  return data;
}

bool add_notify_icon(const notify_icon_id& id,
                     const notify_icon_options& options,
                     DWORD callback_message) {
  auto data = make_data(id, options);
  if (callback_message) {
    data.uFlags |= NIF_MESSAGE;
    data.uCallbackMessage = callback_message;
  }

  if (!Shell_NotifyIconW(NIM_ADD, &data)) {
    return false;
  }

  data.uVersion = NOTIFYICON_VERSION_4;
  return Shell_NotifyIconW(NIM_SETVERSION, &data);
}

bool modify_notify_icon(const notify_icon_id& id,
                        const notify_icon_options& options) {
  auto data = make_data(id, options);
  return Shell_NotifyIconW(NIM_MODIFY, &data);
}

bool delete_notify_icon(const notify_icon_id& id) {
  auto data = make_data(id);
  return Shell_NotifyIconW(NIM_DELETE, &data);
}
