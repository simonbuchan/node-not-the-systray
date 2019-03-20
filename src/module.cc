#include <functional>
#include <unordered_map>

#include "data.hh"
#include "icon-object.hh"
#include "menu-object.hh"
#include "napi/napi.hh"
#include "notify-icon-object.hh"

// https://gist.github.com/paulcbetts/c4e3412262324b551dda
struct IconStreams {
  struct item_t {
    wchar_t exe_path[MAX_PATH];
    int32_t unknown1;
    int32_t unknown2;
  };

  int32_t header_size;
  int32_t unknown1;
  int16_t unknown2;
  int16_t unknown3;
  int32_t count;
  int32_t items_offset;

  item_t items[1];
};

constexpr auto icon_stream_key =
    L"Software\\Classes\\Local Settings\\Software\\"
    L"Microsoft\\Windows\\CurrentVersion\\TrayNotify";
constexpr auto icon_stream_value = L"IconStreams";

napi_value get_icon_stream(napi_env env, napi_callback_info info) {
  DWORD size = 0;
  if (auto error =
          RegGetValueW(HKEY_CURRENT_USER, icon_stream_key, icon_stream_value,
                       RRF_RT_REG_BINARY, nullptr, nullptr, &size);
      error) {
    napi_throw_win32_error(env, "RegGetValueW", error);
    return nullptr;
  }

  void* data = nullptr;
  napi_value result = nullptr;
  NAPI_THROW_RETURN_NULL_IF_NOT_OK(
      env, napi_create_buffer(env, size, &data, &result));

  if (auto error =
          RegGetValueW(HKEY_CURRENT_USER, icon_stream_key, icon_stream_value,
                       RRF_RT_REG_BINARY, nullptr, data, &size);
      error) {
    napi_throw_win32_error(env, "RegGetValueW", error);
    return nullptr;
  }

  return result;
}

napi_value set_icon_stream(napi_env env, napi_callback_info info) {
  napi_buffer_info value;
  NAPI_RETURN_NULL_IF_NOT_OK(napi_get_required_args(env, info, &value));

  if (auto error =
          RegSetKeyValueW(HKEY_CURRENT_USER, icon_stream_key, icon_stream_value,
                          REG_BINARY, value.data, value.size);
      error) {
    napi_throw_win32_error(env, "RegSetKeyValueW", error);
    return nullptr;
  }

  return nullptr;
}

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
                   napi_getter_setter_property("icon_stream", get_icon_stream,
                                               set_icon_stream),
                   napi_value_property("NotifyIcon", notify_icon_constructor),
                   napi_value_property("Menu", menu_constructor),
                   napi_value_property("Icon", icon_constructor),
               }));
  return exports;
}
