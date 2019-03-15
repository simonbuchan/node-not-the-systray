#pragma once

#include "data.hh"
#include "napi/wrap.hh"

struct icon_size_t {
  int32_t width, height;
};

struct IconObject : NapiWrapped<IconObject> {
  HICON icon = nullptr;
  int32_t width = 0;
  int32_t height = 0;
  bool shared = false;

  ~IconObject();

  static napi_status define_class(EnvData* env_data,
                                  napi_value* constructor_value);

  static napi_status load(EnvData* env_data, LPCWSTR path, icon_size_t size,
                          DWORD flags, napi_value* result);
};
