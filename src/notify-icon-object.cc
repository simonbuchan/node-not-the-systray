#include "notify-icon-object.hh"
#include "icon-object.hh"
#include "parse_guid.hh"

#include "notify-icon.hh"

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

// Extends options with ownership values, so the napi_get_value() template
// machinery works.
struct notify_icon_object_options : notify_icon_options {
  struct object_notification_options
      : notify_icon_options::notification_options {
    std::optional<IconObject::Ref> icon_ref;
  };

  std::optional<IconObject::Ref> icon_ref;
  std::optional<object_notification_options> object_notification;
  std::optional<NapiAsyncCallback> select_callback;
};

struct notify_icon_add_options : notify_icon_object_options {
  std::optional<GUID> guid;
  std::optional<bool> replace;
};

napi_status napi_get_value(
    napi_env env, napi_value value,
    notify_icon_object_options::object_notification_options* options) {
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

  if (options->icon_ref) {
    auto icon_object = options->icon_ref.value().wrapped;
    if (!icon_object) {  // Valid, `icon: null`, means that the existing icon is
                         // being cleared.
      options->icon.emplace(nullptr);
    } else {
      options->icon.emplace(icon_object->icon);
      // If this condition is false, then Shell_NotifyIcon() will error.
      options->large_icon = icon_object->width == GetSystemMetrics(SM_CXICON) &&
                            icon_object->height == GetSystemMetrics(SM_CYICON);
    }
  }

  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "title", &options->title));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "text", &options->text));
  return napi_ok;
}

void set_optional_hicon_from_ref(
    std::optional<HICON>& hicon,
    std::optional<NapiUnwrappedRef<IconObject>> const& ref) {
  // Not provided, means don't change existing value: !hicon
  if (!ref) return;

  auto icon_object = ref.value().wrapped;
  if (!icon_object) {
    // `icon: null`, means clear existing value: hicon && !hicon.value()
    hicon.emplace(nullptr);
  } else {
    hicon.emplace(icon_object->icon);
  }
}

napi_status get_icon_options_common(napi_env env, napi_value value,
                                    notify_icon_object_options* options) {
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "icon", &options->icon_ref));

  set_optional_hicon_from_ref(options->icon, options->icon_ref);

  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "tooltip", &options->tooltip));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "hidden", &options->hidden));
  NAPI_RETURN_IF_NOT_OK(napi_get_named_property(env, value, "notification",
                                                &options->object_notification));
  if (options->object_notification) {
    set_optional_hicon_from_ref(options->object_notification->icon,
                                options->object_notification->icon_ref);
    // Deliberate slicing. This is a bit clumsy, but it's the simplest solution
    // that still clearly separates the notify_icon stuff from the N-API
    // ownership stuff.
    options->notification = options->object_notification;
  }
  NAPI_RETURN_IF_NOT_OK(napi_get_named_property(env, value, "onSelect",
                                                &options->select_callback));
  return napi_ok;
}

napi_status napi_get_value(napi_env env, napi_value value,
                           notify_icon_object_options* options) {
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

void apply_options(NotifyIconObject* this_object,
                   notify_icon_object_options& options) {
  if (options.icon_ref) {
    this_object->icon_ref = std::move(options.icon_ref.value());
  }

  if (options.select_callback) {
    this_object->select_callback = std::move(options.select_callback.value());
  }

  if (options.object_notification) {
    if (options.object_notification->icon_ref) {
      this_object->notification_icon_ref =
          std::move(options.object_notification->icon_ref.value());
    } else {  // Since the notification is being replaced, we don't need to keep
              // the old notification icon.
      this_object->notification_icon_ref.clear();
    }
  }
}

napi_status NotifyIconObject::init(napi_env env, napi_callback_info info,
                                   napi_value* result) {
  env_ = env;
  notify_icon_add_options options;
  NAPI_RETURN_IF_NOT_OK(
      napi_get_cb_info(env, info, result, nullptr, 0, &options));

  static int last_id = 0;
  auto id = ++last_id;

  auto env_data = get_env_data(env);

  // Shell_NotifyIcon() fails with "Unspecified error" 0x80004005 if it already
  // exists, including from a previous process that has exited without removing
  // it.
  if (options.guid && options.replace.value_or(true)) {
    // Ignore any errors, as the same error as described above can happen here.
    delete_notify_icon({nullptr, 0, options.guid.value()});
  }

  NAPI_RETURN_IF_NOT_OK(env_data->add_icon(id, *result, this));

  if (!notify_icon.add({env_data->icon_message_loop.hwnd, id, options.guid},
                       options, env_data->icon_message_loop.notify_message())) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return napi_pending_exception;
  }

  apply_options(this, options);

  return napi_ok;
}

napi_value export_NotifyIcon_id(napi_env env, napi_callback_info info) {
  NotifyIconObject* this_object;
  NAPI_RETURN_NULL_IF_NOT_OK(napi_get_this_arg(env, info, &this_object));
  napi_value result;
  NAPI_THROW_RETURN_NULL_IF_NOT_OK(
      env, napi_create(env, this_object->notify_icon.id.callback_id, &result));
  return result;
}

napi_value export_NotifyIcon_update(napi_env env, napi_callback_info info) {
  NotifyIconObject* this_object;
  notify_icon_object_options options;
  NAPI_RETURN_NULL_IF_NOT_OK(
      napi_get_cb_info(env, info, &this_object, nullptr, 1, &options));

  if (!this_object->notify_icon.modify(options)) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return nullptr;
  }
  apply_options(this_object, options);

  return nullptr;
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

napi_status NotifyIconObject::remove(napi_env env) {
  if (!notify_icon.id) {
    return napi_ok;
  }

  auto env_data = get_env_data(env);

  auto id = notify_icon.id.callback_id;

  if (!notify_icon.clear()) {
    napi_throw_win32_error(env, "Shell_NotifyIconW");
    return napi_pending_exception;
  }

  env_data->remove_icon(id);

  return napi_ok;
}
