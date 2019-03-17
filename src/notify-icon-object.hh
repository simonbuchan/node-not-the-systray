#pragma once

#include "data.hh"
#include "napi/wrap.hh"

struct NotifyIconObject : NapiWrapped<NotifyIconObject> {
  static napi_status define_class(EnvData* env_data,
                                  napi_value* constructor_value);

  int32_t id;

 private:
  friend NapiWrapped;
  napi_status init(napi_env env, napi_callback_info info, napi_value* result);
};
