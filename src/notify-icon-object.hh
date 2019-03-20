#pragma once

#include "data.hh"
#include "icon-object.hh"
#include "notify-icon.hh"
#include "napi/wrap.hh"

struct NotifyIconObject : NapiWrapped<NotifyIconObject> {
  static napi_status define_class(EnvData* env_data,
                                  napi_value* constructor_value);

  napi_env env_ = nullptr;
  NotifyIcon notify_icon;
  bool large_balloon_icon = false;
  NapiUnwrappedRef<IconObject> icon_ref;
  NapiUnwrappedRef<IconObject> notification_icon_ref;
  NapiAsyncCallback select_callback;

  napi_status select(napi_env env, napi_value this_value, bool right_button,
                     int16_t mouse_x, int16_t mouse_y);

  napi_status remove(napi_env env);

 private:
  friend NapiWrapped;
  napi_status init(napi_env env, napi_callback_info info, napi_value* result);
};
