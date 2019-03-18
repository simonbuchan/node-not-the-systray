#include "notify-icon-object.hh"
#include "icon-object.hh"
#include "parse_guid.hh"

#include <shellapi.h>

using namespace std::string_literals;

template <size_t N>
char* copy(std::string_view sv, char (&dest)[N]) {
  return std::copy(std::begin(sv), std::end(sv), dest);
}

template <size_t N>
char16_t* copy(std::u16string_view sv, char16_t (&dest)[N]) {
  return std::copy(std::begin(sv), std::end(sv), dest);
}

template <size_t N>
wchar_t* copy(std::wstring_view sv, wchar_t (&dest)[N]) {
  return std::copy(std::begin(sv), std::end(sv), dest);
}

napi_status napi_get_value(napi_env env, napi_value value, GUID* result) {
  bool is_buffer;
  NAPI_RETURN_IF_NOT_OK(napi_is_buffer(env, value, &is_buffer));

  if (is_buffer) {
    void* data;
    size_t size = sizeof(*result);
    NAPI_RETURN_IF_NOT_OK(napi_get_buffer_info(env, value, &data, &size));
    if (size != sizeof(*result)) {
      NAPI_RETURN_IF_NOT_OK(napi_throw_error(
          env, nullptr, "invalid guid buffer: must be 16 bytes."));
      return napi_pending_exception;
    }
    memcpy(result, data, size);
    return napi_ok;
  }

  std::string guid;
  NAPI_RETURN_IF_NOT_OK(napi_get_value(env, value, &guid));

  switch (parse_guid(guid, result)) {
    case parse_guid_result::ok:
      return napi_ok;
    default:
      NAPI_RETURN_IF_NOT_OK(napi_throw_error(env, nullptr, "invalid guid."));
      return napi_pending_exception;
    case parse_guid_result::bad_size:
      NAPI_RETURN_IF_NOT_OK(
          napi_throw_error(env, nullptr, "invalid guid: must be 36 chars."));
      return napi_pending_exception;
    case parse_guid_result::bad_hyphens:
      NAPI_RETURN_IF_NOT_OK(napi_throw_error(
          env, nullptr,
          "invalid guid: must have '-' at offsets 8, 13, 18 and 23."));
      return napi_pending_exception;
    case parse_guid_result::bad_hex:
      NAPI_RETURN_IF_NOT_OK(napi_throw_error(
          env, nullptr, "invalid guid: invalid hexadecimal characters."));
      return napi_pending_exception;
  }
}

napi_status napi_get_value(napi_env env, napi_value value,
                           NapiAsyncCallback* result) {
  napi_valuetype type;
  NAPI_RETURN_IF_NOT_OK(napi_typeof(env, value, &type));

  switch (type) {
    default:
      napi_throw_type_error(env, nullptr,
                            "Expected function, undefined, or null");
      return napi_function_expected;

    case napi_undefined:
    case napi_null:
      result->clear();
      return napi_ok;

    case napi_function:
      return result->create(env, value);
  }
}

struct notification_options {
  // uFlags: NIF_*
  std::optional<bool> realtime;
  // dwInfoFlags: NIIF_*
  std::optional<bool> sound;
  std::optional<bool> respect_quiet_time;

  std::optional<IconObject::Ref> icon_ref;
  std::optional<std::wstring> title;
  std::optional<std::wstring> text;
};

// Represents options common to addIcon() and modifyIcon()
struct notify_icon_options {
  // dwState: NIS_*
  std::optional<bool> hidden;
  std::optional<bool> large_balloon_icon;

  std::optional<IconObject::Ref> icon_ref;
  std::optional<std::wstring> tooltip;
  std::optional<notification_options> notification;

  std::optional<NapiAsyncCallback> select_callback;
};

struct notify_icon_add_options : notify_icon_options {
  std::optional<bool> replace;
};

napi_status napi_get_value(napi_env env, napi_value value,
                           notification_options* options) {
  napi_valuetype type;
  NAPI_RETURN_IF_NOT_OK(napi_typeof(env, value, &type));

  if (type == napi_undefined || type == napi_null) {
    options->title = L""sv;
    options->text = L""sv;
    return napi_ok;
  }

  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "sound", &options->sound));
  NAPI_RETURN_IF_NOT_OK(napi_get_named_property(env, value, "respectQuietTime",
                                                &options->respect_quiet_time));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "realtime", &options->realtime));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "icon", &options->icon_ref));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "title", &options->title));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "text", &options->text));
  return napi_ok;
}

napi_status get_icon_options_common(napi_env env, napi_value value,
                                    notify_icon_options* options) {
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "icon", &options->icon_ref));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "tooltip", &options->tooltip));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "hidden", &options->hidden));
  NAPI_RETURN_IF_NOT_OK(napi_get_named_property(env, value, "notification",
                                                &options->notification));
  NAPI_RETURN_IF_NOT_OK(napi_get_named_property(env, value, "onSelect",
                                                &options->select_callback));
  return napi_ok;
}

napi_status napi_get_value(napi_env env, napi_value value,
                           notify_icon_options* options) {
  napi_valuetype type;
  NAPI_RETURN_IF_NOT_OK(napi_typeof(env, value, &type));

  if (type == napi_undefined || type == napi_null) {
    return napi_ok;
  }

  return get_icon_options_common(env, value, options);
}

napi_status napi_get_value(napi_env env, napi_value value,
                           notify_icon_add_options* options) {
  napi_valuetype type;
  NAPI_RETURN_IF_NOT_OK(napi_typeof(env, value, &type));

  if (type == napi_undefined || type == napi_null) {
    return napi_ok;
  }

  NAPI_RETURN_IF_NOT_OK(get_icon_options_common(env, value, options));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "replace", &options->replace));
  return napi_ok;
}

void apply_notification(notification_options* options, NOTIFYICONDATAW* nid,
                        NotifyIconObject* this_object) {
  nid->uFlags |= NIF_INFO;
  if (options->realtime.value_or(false)) nid->uFlags |= NIF_REALTIME;
  if (!options->sound.value_or(true)) nid->dwInfoFlags |= NIIF_NOSOUND;
  if (options->respect_quiet_time.value_or(true))
    nid->dwInfoFlags |= NIIF_RESPECT_QUIET_TIME;

  if (options->icon_ref) {
    this_object->notification_icon_ref = std::move(options->icon_ref.value());
    nid->dwInfoFlags |= NIIF_USER;
    auto icon_object = this_object->notification_icon_ref.wrapped;
    nid->hBalloonIcon = icon_object->icon;
    if (icon_object->width == GetSystemMetrics(SM_CXICON) &&
        icon_object->height == GetSystemMetrics(SM_CYICON)) {
      nid->dwInfoFlags |= NIIF_LARGE_ICON;
    }
  } else {
    this_object->notification_icon_ref = {};
  }

  copy(options->title.value_or(L""s), nid->szInfoTitle);
  copy(options->text.value_or(L""s), nid->szInfo);
}

void apply_options(notify_icon_options* options, NOTIFYICONDATAW* nid,
                   NotifyIconObject* this_object) {
  if (options->hidden) {
    nid->uFlags |= NIF_STATE;
    nid->dwStateMask |= NIS_HIDDEN;
    if (options->hidden.value()) nid->dwState |= NIS_HIDDEN;
  }

  if (options->icon_ref) {
    nid->uFlags |= NIF_ICON;
    this_object->icon_ref = std::move(options->icon_ref.value());

    nid->hIcon = this_object->icon_ref.wrapped->icon;
  }

  if (options->tooltip) {
    nid->uFlags |= NIF_TIP | NIF_SHOWTIP;
    copy(options->tooltip.value(), nid->szTip);
  }

  if (options->select_callback)
    this_object->select_callback = std::move(options->select_callback.value());

  if (options->notification)
    apply_notification(&options->notification.value(), nid, this_object);
}

napi_status NotifyIconObject::init(napi_env env, napi_callback_info info,
                                   napi_value* result) {
  notify_icon_add_options options;
  NAPI_RETURN_IF_NOT_OK(
      napi_get_cb_info(env, info, result, nullptr, 1, &guid, &options));

  static int last_id = 0;
  id = ++last_id;

  auto env_data = get_env_data(env);

  env_data->add_icon(id, *result, this);

  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.uID = id;
  nid.guidItem = guid;
  nid.hWnd = env_data->msg_hwnd;
  nid.uFlags = NIF_GUID | NIF_MESSAGE;
  nid.uCallbackMessage = WM_TRAYICON_CALLBACK;

  apply_options(&options, &nid, this);

  // Shell_NotifyIcon() fails with "Unspecified error" 0x80004005 if it already
  // exists, including from a previous process that has exited without removing
  // it.
  if (options.replace.value_or(true)) {
    // Ignore any errors, as the same error as described above can happen here.
    Shell_NotifyIcon(NIM_DELETE, &nid);
  }

  // Shell_NotifyIcon() isn't documented to return errors via GetLastError(),
  // but it seems like it does... sort of. Just in case, clear it.
  SetLastError(0);

  if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return napi_pending_exception;
  }

  // Receive new-style window messages.
  nid.uVersion = NOTIFYICON_VERSION_4;
  if (!Shell_NotifyIcon(NIM_SETVERSION, &nid)) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return napi_pending_exception;
  }

  return napi_ok;
}

napi_value export_NotifyIcon_id(napi_env env, napi_callback_info info) {
  NotifyIconObject* this_object;
  NAPI_RETURN_NULL_IF_NOT_OK(napi_get_this_arg(env, info, &this_object));
  napi_value result;
  NAPI_THROW_RETURN_NULL_IF_NOT_OK(env,
                                   napi_create(env, this_object->id, &result));
  return result;
}

napi_value export_NotifyIcon_update(napi_env env, napi_callback_info info) {
  NotifyIconObject* this_object;
  notify_icon_options options;
  NAPI_RETURN_NULL_IF_NOT_OK(
      napi_get_cb_info(env, info, &this_object, nullptr, 1, &options));

  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.guidItem = this_object->guid;
  nid.uFlags = NIF_GUID;

  apply_options(&options, &nid, this_object);

  // Shell_NotifyIcon() isn't documented to return errors via GetLastError(),
  // but it seems like it sometimes returns something valid. Clear it here
  // just so we don't have a chance of returning an invalid error message.
  SetLastError(0);

  if (!Shell_NotifyIcon(NIM_MODIFY, &nid)) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return nullptr;
  }
  return nullptr;
}

napi_status NotifyIconObject::remove(napi_env env) {
  auto env_data = get_env_data(env);

  NOTIFYICONDATAW nid = {sizeof(nid)};
  nid.guidItem = guid;
  nid.uFlags = NIF_GUID;

  if (!Shell_NotifyIcon(NIM_DELETE, &nid)) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return napi_pending_exception;
  }

  env_data->remove_icon(id);

  return napi_ok;
}

napi_value export_NotifyIcon_remove(napi_env env, napi_callback_info info) {
  NotifyIconObject* this_object;
  NAPI_RETURN_NULL_IF_NOT_OK(napi_get_this_arg(env, info, &this_object));
  NAPI_RETURN_NULL_IF_NOT_OK(this_object->remove(env));
  return nullptr;
}

napi_status NotifyIconObject::define_class(EnvData* env_data,
                                           napi_value* constructor_value) {
  return NapiWrapped::define_class(
      env_data->env, "NotifyIcon", constructor_value,
      &env_data->notify_icon_constructor,
      {
          napi_getter_property("id", export_NotifyIcon_id),
          napi_method_property("update", export_NotifyIcon_update),
          napi_method_property("remove", export_NotifyIcon_remove),
      });
}

napi_status NotifyIconObject::select(napi_env env, napi_value this_value,
                                     bool right_button, int16_t mouse_x,
                                     int16_t mouse_y) {
  napi_value event;
  NAPI_RETURN_IF_NOT_OK(napi_create_object(env, &event,
                                           {
                                               {"target", this_value},
                                               {"rightButton", right_button},
                                               {"mouseX", mouse_x},
                                               {"mouseY", mouse_y},
                                           }));

  if (select_callback(this_value, {event}) == nullptr) {
    return napi_generic_failure;
  }

  return napi_ok;
}
