/**
 * Some simple C++ wrappers that simplify node_api.h usage.
 * Less magic than node-addon-api's napi.h, which is both
 * good and bad, as neither API is that well documented.
 * This is me learning it properly.
 */
#pragma once

#include <string>

#include "napi/async.hh"
#include "napi/core.hh"
#include "napi/props.hh"
#include "napi/value.hh"
#include "napi/win32.hh"
#include "napi/wrap.hh"

template <typename This, typename... Args>
inline napi_status napi_get_cb_info(napi_env env, napi_callback_info info,
                                    This* this_arg, void** data, int required,
                                    Args*... args) {
  napi_value this_value = nullptr;
  napi_value arg_values[sizeof...(args)];
  size_t argc = sizeof...(args);
  NAPI_RETURN_IF_NOT_OK(napi_get_cb_info(
      env, info, &argc, arg_values, this_arg ? &this_value : nullptr, data));

  if (argc < required) {
    napi_throw_error(
        env, nullptr,
        ("Invalid args: provided "s + std::to_string(argc) + " but requires "s +
         std::to_string(required) + " arguments."s)
            .c_str());
    return napi_invalid_arg;
  }

  if (this_arg) {
    if (auto status = napi_get_value(env, this_value, this_arg);
        status != napi_ok) {
      return napi_rethrow_with_location(env, "this parameter"sv);
    }
  }

  if (auto [status, status_value] =
          napi_get_many_values(env, arg_values, args...);
      status != napi_ok) {
    return napi_rethrow_with_location(env, "parameter "s + std::to_string(status_value - arg_values + 1));
  }

  return napi_ok;
}

template <typename... Args>
inline napi_status napi_get_cb_info(napi_env env, napi_callback_info info,
                                    nullptr_t this_arg, void** data,
                                    int required, Args*... args) {
  return napi_get_cb_info<void, Args...>(env, info, this_arg, data, required,
                                         args...);
}

template <typename... Args>
inline napi_status napi_get_required_args(napi_env env, napi_callback_info info,
                                          Args*... args) {
  return napi_get_cb_info(env, info, nullptr, nullptr, sizeof...(args),
                          args...);
}

template <typename... Args>
inline napi_status napi_get_args(napi_env env, napi_callback_info info,
                                 int required_args, Args*... args) {
  return napi_get_cb_info(env, info, nullptr, nullptr, required_args, args...);
}

#if EXCEPTIONS
// This doesn't work at all for some reason?
#define EXPORT_METHOD(name) export_wrap_method_property(env, #name, name)

struct napi_exception {
  napi_value value;
};

inline void wrap_callback(napi_env env, napi_callback callback,
                          napi_callback* callback_result, void** data_result) {
  *data_result = callback;
  *callback_result = [](napi_env env, napi_callback_info info) -> napi_value {
    napi_callback data;
    if (napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**)&data) !=
        napi_ok) {
      return nullptr;
    }

    try {
      return data(env, info);
    } catch (napi_exception& e) {
      napi_throw(env, e.value);
      return nullptr;
    } catch (...) {
      return nullptr;
    }
  };
}

inline napi_property_descriptor export_wrap_method_property(
    napi_env env, const char* utf8name, napi_callback method) {
  napi_callback wrapped_method;
  void* data;
  wrap_callback(env, method, &wrapped_method, &data);
  return export_method_property(utf8name, wrapped_method, data);
}
#endif
