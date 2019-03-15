#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <node_api.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

#define NAPI_RETURN_IF_NOT_OK(napi_expr)             \
  do {                                               \
    if (napi_status _napi_expr_status = (napi_expr); \
        _napi_expr_status != napi_ok) {              \
      return _napi_expr_status;                      \
    }                                                \
  } while (false)

#define NAPI_RETURN_NULL_IF_NOT_OK(napi_expr)        \
  do {                                               \
    if (napi_status _napi_expr_status = (napi_expr); \
        _napi_expr_status != napi_ok) {              \
      return nullptr;                                \
    }                                                \
  } while (false)

#define NAPI_RETURN_INIT_IF_NOT_OK(napi_expr, ...)   \
  do {                                               \
    if (napi_status _napi_expr_status = (napi_expr); \
        _napi_expr_status != napi_ok) {              \
      return {_napi_expr_status, __VA_ARGS__};       \
    }                                                \
  } while (false)

#define NAPI_THROW_RETURN_STATUS_IF_NOT_OK(env, napi_expr) \
  do {                                                     \
    if (napi_status _napi_expr_status = (napi_expr);       \
        _napi_expr_status != napi_ok) {                    \
      return napi_throw_last_error(env);                   \
    }                                                      \
  } while (false)

#define NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, napi_expr) \
  do {                                                   \
    if (napi_status _napi_expr_status = (napi_expr);     \
        _napi_expr_status != napi_ok) {                  \
      napi_throw_last_error(env);                        \
      return;                                            \
    }                                                    \
  } while (false)

#define NAPI_THROW_RETURN_NULL_IF_NOT_OK(env, napi_expr) \
  do {                                                   \
    if (napi_status _napi_expr_status = (napi_expr);     \
        _napi_expr_status != napi_ok) {                  \
      napi_throw_last_error(env);                        \
      return nullptr;                                    \
    }                                                    \
  } while (false)

#define NAPI_FATAL_IF(expr)      \
  do {                           \
    if (expr) NAPI_FATAL(#expr); \
  } while (false)

#define NAPI_FATAL_IF_NOT_OK(napi_expr)              \
  do {                                               \
    if (napi_status _napi_expr_status = (napi_expr); \
        _napi_expr_status != napi_ok)                \
      NAPI_FATAL(#napi_expr);                        \
  } while (false)

#define NAPI_FATAL(msg) \
  napi_fatal_error(__FUNCTION__, NAPI_AUTO_LENGTH, msg, NAPI_AUTO_LENGTH)

napi_status napi_throw_last_error(napi_env env);

napi_status napi_rethrow_with_location(napi_env env, std::string_view location);
