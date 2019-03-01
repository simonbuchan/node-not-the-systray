#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <optional>

#include <node_api.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

#define NAPI_RETURN_IF_NOT_OK(napi_expr) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) { \
            return _napi_expr_status; \
        } \
    } while (false)

#define NAPI_RETURN_NULL_IF_NOT_OK(napi_expr) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) { \
            return nullptr; \
        } \
    } while (false)

#define NAPI_RETURN_INIT_IF_NOT_OK(napi_expr, ...) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) { \
            return { _napi_expr_status, __VA_ARGS__ }; \
        } \
    } while (false)

#define NAPI_THROW_RETURN_STATUS_IF_NOT_OK(env, napi_expr) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) { \
            return napi_throw_last_error(env); \
        } \
    } while (false)

#define NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, napi_expr) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) { \
            napi_throw_last_error(env); \
            return; \
        } \
    } while (false)

#define NAPI_THROW_RETURN_NULL_IF_NOT_OK(env, napi_expr) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) { \
            napi_throw_last_error(env); \
            return nullptr; \
        } \
    } while (false)

#define NAPI_FATAL_IF(napi_expr) \
    do { \
        if (napi_expr) NAPI_FATAL(#napi_expr); \
    } while (false)

#define NAPI_FATAL_IF_NOT_OK(napi_expr) \
    do { \
        if (napi_status _napi_expr_status = (napi_expr); _napi_expr_status != napi_ok) \
            NAPI_FATAL(#napi_expr); \
    } while (false)

#define NAPI_FATAL(msg) \
    napi_fatal_error(__FUNCTION__, NAPI_AUTO_LENGTH, msg, NAPI_AUTO_LENGTH)

inline napi_status napi_throw_last_error(napi_env env)
{
    bool is_pending = false;
    NAPI_FATAL_IF_NOT_OK(napi_is_exception_pending(env, &is_pending));
    if (is_pending)
    {
        return napi_pending_exception;
    }

    const napi_extended_error_info* info = nullptr;
    NAPI_FATAL_IF_NOT_OK(napi_get_last_error_info(env, &info));

    switch (info->error_code)
    {
        case napi_ok:
        case napi_pending_exception:
            return info->error_code;
    
        case napi_object_expected:
        case napi_string_expected:
        case napi_name_expected:
        case napi_function_expected:
        case napi_number_expected:
        case napi_boolean_expected:
        case napi_array_expected:
        case napi_bigint_expected:
            NAPI_FATAL_IF_NOT_OK(napi_throw_type_error(env, nullptr, info->error_message));
            return napi_pending_exception;

        default:
            NAPI_FATAL_IF_NOT_OK(napi_throw_error(env, nullptr, info->error_message));
            return napi_pending_exception;
    }
}
