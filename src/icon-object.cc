#include "icon-object.hh"

napi_status napi_get_value(napi_env env, napi_value value,
                           icon_size_t* result) {
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "width", &result->width));
  NAPI_RETURN_IF_NOT_OK(
      napi_get_named_property(env, value, "height", &result->height));
  return napi_ok;
}

napi_value export_Icon_loadBuiltin(napi_env env, napi_callback_info info) {
  uint32_t id;
  icon_size_t size;
  NAPI_RETURN_NULL_IF_NOT_OK(napi_get_required_args(env, info, &id, &size));

  auto env_data = get_env_data(env);
  napi_value result;
  NAPI_RETURN_NULL_IF_NOT_OK(IconObject::load(env_data, MAKEINTRESOURCE(id),
                                              size, LR_SHARED, &result));
  return result;
}

napi_value export_Icon_loadFile(napi_env env, napi_callback_info info) {
  std::wstring path;
  icon_size_t size;
  NAPI_RETURN_NULL_IF_NOT_OK(napi_get_required_args(env, info, &path, &size));

  auto env_data = get_env_data(env);
  napi_value result;
  NAPI_RETURN_NULL_IF_NOT_OK(
      IconObject::load(env_data, path.c_str(), size, LR_LOADFROMFILE, &result));
  return result;
}

napi_property_descriptor system_metric_property(
    const char* utf8name, int metric,
    napi_property_attributes attributes = napi_enumerable) {
  auto getter = [](napi_env env, napi_callback_info info) -> napi_value {
    void* data;
    NAPI_THROW_RETURN_NULL_IF_NOT_OK(
        env, napi_get_cb_info(env, info, nullptr, nullptr, nullptr, &data));
    auto value = GetSystemMetrics((int)(size_t)data);
    napi_value result;
    NAPI_THROW_RETURN_NULL_IF_NOT_OK(env, napi_create(env, value, &result));
    return result;
  };

  return napi_getter_property(utf8name, getter, attributes,
                              (void*)(size_t)metric);
}

template <auto IconObject::*member>
napi_property_descriptor member_getter_property(
    const char* utf8name,
    napi_property_attributes attributes = napi_enumerable) {
  auto getter = [](napi_env env, napi_callback_info info) -> napi_value {
    napi_value this_value;
    NAPI_THROW_RETURN_NULL_IF_NOT_OK(
        env,
        napi_get_cb_info(env, info, nullptr, nullptr, &this_value, nullptr));

    if (auto [status, wrapped] = IconObject::try_unwrap(env, this_value);
        status != napi_ok) {
      napi_throw_last_error(env);
      return nullptr;
    } else if (!wrapped) {
      napi_throw_type_error(env, nullptr,
                            "'this' must be an instance of Icon.");
      return nullptr;
    } else {
      napi_value result;
      NAPI_THROW_RETURN_NULL_IF_NOT_OK(
          env, napi_create(env, wrapped->*member, &result));
      return result;
    }
  };

  return napi_getter_property(utf8name, getter, attributes, nullptr);
}

IconObject::~IconObject() {
  if (!shared) {
    DestroyIcon(icon);
  }
}

napi_status IconObject::define_class(EnvData* env_data,
                                     napi_value* constructor_value) {
  auto env = env_data->env;
  napi_value small_value, large_value;
  NAPI_RETURN_IF_NOT_OK(napi_create_object(
      env, &small_value,
      {
          system_metric_property("width", SM_CXSMICON, napi_static),
          system_metric_property("height", SM_CYSMICON, napi_static),
      }));
  NAPI_RETURN_IF_NOT_OK(napi_create_object(
      env, &large_value,
      {
          system_metric_property("width", SM_CXICON, napi_static),
          system_metric_property("height", SM_CYICON, napi_static),
      }));

  return NapiWrapped::define_class(
      env, "Icon", constructor_value, &env_data->icon_constructor,
      {
          napi_value_property("small", small_value, napi_static),
          napi_value_property("large", large_value, napi_static),
          napi_method_property("loadBuiltin", export_Icon_loadBuiltin,
                               napi_static),
          napi_method_property("loadFile", export_Icon_loadFile, napi_static),

          member_getter_property<&IconObject::width>("width"),
          member_getter_property<&IconObject::height>("height"),
      });
}

napi_status IconObject::load(EnvData* env_data, LPCWSTR path, icon_size_t size,
                             DWORD flags, napi_value* result) {
  auto env = env_data->env;
  auto shared = (flags & LR_SHARED) != 0;

  auto icon = (HICON)LoadImageW(nullptr, path, IMAGE_ICON, size.width,
                                size.height, flags);

  if (!icon) {
    napi_throw_win32_error(env, "LoadImageW");
    return napi_pending_exception;
  }

  if (auto [status, wrapper, wrapped] =
          NapiWrapped::new_instance(env, env_data->icon_constructor);
      status != napi_ok) {
    if (!shared) {
      DestroyIcon(icon);
    }

    napi_throw_last_error(env);
    return napi_pending_exception;
  } else {
    wrapped->icon = icon;
    wrapped->shared = shared;
    wrapped->width = size.width;
    wrapped->height = size.height;
    *result = wrapper;
    return napi_ok;
  }
}
