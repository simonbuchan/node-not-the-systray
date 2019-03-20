#pragma once

// Prevent pulling in winsock.h in windows.h, which breaks uv.h
#define WIN32_LEAN_AND_MEAN

#include <functional>
#include <unordered_map>

#include <Windows.h>
#include <uv.h>

#include "napi/napi.hh"
#include "unique.hh"

constexpr UINT WM_TRAYICON_CALLBACK = WM_USER + 123;

using WndHandle = Unique<HWND, DestroyWindow>;

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
  WndHandle msg_hwnd;
  std::unordered_map<int32_t, IconData> icons;
  uv_idle_t message_pump_idle = {this};

  napi_status add_icon(int32_t id, napi_value value, NotifyIconObject* object);
  bool remove_icon(int32_t id);

  ~EnvData();
};

EnvData* get_env_data(napi_env env);

std::tuple<napi_status, EnvData*> create_env_data(napi_env env);
