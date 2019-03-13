#include "icon-object.hh"

napi_value export_Icon_loadBuiltin(napi_env env, napi_callback_info info) {
  uint32_t id;
  int32_t width, height;
  NAPI_RETURN_NULL_IF_NOT_OK(
      napi_get_required_args(env, info, &id, &width, &height));

  auto env_data = get_env_data(env);
  napi_value result;
  NAPI_RETURN_NULL_IF_NOT_OK(IconObject::load(
      env_data, MAKEINTRESOURCE(id), width, height, LR_SHARED, &result));
  return result;
}

napi_value export_Icon_loadFile(napi_env env, napi_callback_info info) {
  std::wstring path;
  int32_t width, height;
  NAPI_RETURN_NULL_IF_NOT_OK(
      napi_get_required_args(env, info, &path, &width, &height));

  auto env_data = get_env_data(env);
  napi_value result;
  NAPI_RETURN_NULL_IF_NOT_OK(IconObject::load(
      env_data, path.c_str(), width, height, LR_LOADFROMFILE, &result));
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
  return NapiWrapped::define_class(
      env_data->env, "Icon", constructor_value, &env_data->icon_constructor,
      {
          system_metric_property("smallWidth", SM_CXSMICON, napi_static),
          system_metric_property("smallHeight", SM_CYSMICON, napi_static),
          system_metric_property("largeWidth", SM_CXICON, napi_static),
          system_metric_property("largeHeight", SM_CYICON, napi_static),
          napi_method_property("loadBuiltin", export_Icon_loadBuiltin,
                               napi_static),
          napi_method_property("loadFile", export_Icon_loadFile, napi_static),

          member_getter_property<&IconObject::width>("width"),
          member_getter_property<&IconObject::height>("height"),
      });
}

napi_status IconObject::load(EnvData* env_data, LPCWSTR path, int32_t width,
                             int32_t height, DWORD flags, napi_value* result) {
  auto env = env_data->env;
  auto shared = (flags & LR_SHARED) != 0;

  auto icon =
      (HICON)LoadImageW(nullptr, path, IMAGE_ICON, width, height, flags);

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
    wrapped->width = width;
    wrapped->height = height;
    *result = wrapper;
    return napi_ok;
  }
}
