#include <functional>
#include <unordered_map>

#include "data.hh"
#include "icon-object.hh"
#include "menu-object.hh"
#include "napi/napi.hh"
#include "notify-icon-object.hh"

NAPI_MODULE_INIT() {
  EnvData* env_data;
  if (auto [status, new_env_data] = create_env_data(env); status != napi_ok) {
    napi_throw_last_error(env);
    return nullptr;
  } else {
    env_data = new_env_data;
  }

  napi_value notify_icon_constructor, menu_constructor, icon_constructor;
  NAPI_THROW_RETURN_NULL_IF_NOT_OK(
      env, NotifyIconObject::define_class(env_data, &notify_icon_constructor));
  NAPI_THROW_RETURN_NULL_IF_NOT_OK(
      env, MenuObject::define_class(env_data, &menu_constructor));
  NAPI_THROW_RETURN_NULL_IF_NOT_OK(
      env, IconObject::define_class(env_data, &icon_constructor));

  NAPI_THROW_RETURN_NULL_IF_NOT_OK(
      env, napi_define_properties(
               env, exports,
               {
                   napi_value_property("NotifyIcon", notify_icon_constructor),
                   napi_value_property("Menu", menu_constructor),
                   napi_value_property("Icon", icon_constructor),
               }));
  return exports;
}
