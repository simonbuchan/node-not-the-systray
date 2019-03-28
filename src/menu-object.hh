#pragma once

#include "data.hh"
#include "napi/wrap.hh"
#include "unique.hh"

using MenuHandle = Unique<HMENU, DestroyMenu>;

struct MenuObject : NapiWrapped<MenuObject> {
  MenuHandle menu;

  static napi_status define_class(EnvData* env_data,
                                  napi_value* constructor_value);
  static napi_status new_instance(EnvData* env_data, MenuHandle menu,
                                  napi_value* result);

 protected:
  friend NapiWrapped;
  napi_status init(napi_env env, napi_callback_info info, napi_value* result);
};
