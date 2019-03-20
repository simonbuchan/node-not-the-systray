#pragma once

#include <optional>
#include <string>

#include <Windows.h>

struct notify_icon_id {
  HWND callback_hwnd = nullptr;
  int32_t callback_id = 0;
  std::optional<GUID> guid;

  explicit operator bool() const {
    return callback_id || guid;
  }
};

// Represents options common to add_ and modify_notify_icon.
struct notify_icon_options {
  struct notification_options {
    // uFlags: NIF_*
    std::optional<bool> realtime;
    // dwInfoFlags: NIIF_*
    std::optional<bool> sound;
    std::optional<bool> respect_quiet_time;

    std::optional<HICON> icon;
    bool large_icon;
    std::optional<std::wstring> title;
    std::optional<std::wstring> text;
  };

  // dwState: NIS_*
  std::optional<bool> hidden;

  std::optional<HICON> icon;
  std::optional<std::wstring> tooltip;
  std::optional<notification_options> notification;
};

bool add_notify_icon(const notify_icon_id& id,
                     const notify_icon_options& options,
                     DWORD callback_message);
bool modify_notify_icon(const notify_icon_id& id,
                        const notify_icon_options& options);
bool delete_notify_icon(const notify_icon_id& id);

struct NotifyIcon {
  notify_icon_id id;

  NotifyIcon() = default;
  NotifyIcon(const NotifyIcon&) = delete;
  NotifyIcon& operator=(const NotifyIcon&) = delete;
  NotifyIcon(NotifyIcon&& other) noexcept : id{std::exchange(other.id, {})} {}
  NotifyIcon& operator=(NotifyIcon&& other) noexcept {
    std::swap(id, other.id);
    return *this;
  }

  bool add(const notify_icon_id& new_id, const notify_icon_options& options,
           DWORD callback_message) {
    clear();
    if (!add_notify_icon(new_id, options, callback_message)) {
      return false;
    }
    id = new_id;
    return true;
  }

  bool modify(const notify_icon_options& options) {
    return modify_notify_icon(id, options);
  }

  bool clear() {
    if (!id) return true;
    auto old_id = std::exchange(id, {});
    return delete_notify_icon(old_id);
  }

  ~NotifyIcon() { clear(); }
};
