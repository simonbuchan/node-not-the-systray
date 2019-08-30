#pragma once

#include "napi/napi.hh"
#include "notify-icon-message-loop.hh"

struct NotifyIconObject;

struct EnvData {
  struct IconData {
    napi_ref ref;
    int32_t id;
    std::optional<GUID> guid;
  };

  // Note that there's not much point trying to clean up these on
  // being destroyed, as that is when the environment is being destroyed.
  napi_env env = nullptr;
  napi_ref notify_icon_constructor = nullptr;
  napi_ref menu_constructor = nullptr;
  napi_ref icon_constructor = nullptr;

  std::unordered_map<int32_t, IconData> icons;
  NotifyIconMessageLoop icon_message_loop;

  napi_status add_icon(int32_t id, napi_value value, NotifyIconObject* object);
  bool remove_icon(int32_t id);

  struct NotifySelectArgs {
    bool right_button = false;
    int32_t mouse_x = 0;
    int32_t mouse_y = 0;
  };
  void notify_select(int32_t id, NotifySelectArgs args);

  ~EnvData();
};

EnvData* get_env_data(napi_env env);

std::tuple<napi_status, EnvData*> create_env_data(napi_env env);
