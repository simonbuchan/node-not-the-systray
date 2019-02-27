/**
 * Some simple C++ wrappers that simplify node_api.h usage.
 * Less magic than node-addon-api's napi.h, which is both
 * good and bad, as neither API is that well documented.
 * This is me learning it properly.
 */
#pragma once

#include "napi-core.hh"
#include "napi-async.hh"
#include "napi-props.hh"
#include "napi-value.hh"
#include "napi-win32.hh"

template <typename... T>
inline napi_status napi_get_required_args(napi_env env, napi_callback_info info, T*... results)
{
    return napi_get_args(env, info, sizeof...(results), results...);
}

template <typename... T>
inline napi_status napi_get_args(napi_env env, napi_callback_info info, int required, T*... results)
{
    napi_value argv[sizeof...(results)];
    size_t argc = sizeof...(results);
    NAPI_RETURN_IF_NOT_OK(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    if (argc < required)
    {
        napi_throw_error(env, nullptr, (
            "invalid args: provided "s + std::to_string(argc) +
            " but requires "s + std::to_string(required) +
            " arguments."s
        ).c_str());
        return napi_invalid_arg;
    }

    if (auto [status, status_argp] = napi_get_many_values(env, argv, results...); status != napi_ok)
    {
        if (status == napi_pending_exception)
            return status;

        const napi_extended_error_info* errorinfo;
        NAPI_RETURN_IF_NOT_OK(napi_get_last_error_info(env, &errorinfo));

        auto message = "Invalid argument "s + std::to_string(status_argp - argv);
        if (errorinfo->error_message) message += ": "s + errorinfo->error_message;
        napi_throw_error(env, nullptr, message.c_str());
        return status;
    }

    return napi_ok;
}

#if EXCEPTIONS
// This doesn't work at all for some reason?
#define EXPORT_METHOD(name) export_wrap_method_property(env, #name, name)

struct napi_exception { napi_value value; };

inline void wrap_callback(
    napi_env env,
    napi_callback callback,
    napi_callback *callback_result,
    void **data_result)
{
    *data_result = callback;
    *callback_result = [](napi_env env, napi_callback_info info) -> napi_value {
        napi_callback data;
        if (napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &data) != napi_ok) { return nullptr; }

        try
        {
            return data(env, info);
        }
        catch (napi_exception& e)
        {
            napi_throw(env, e.value);
            return nullptr;
        }
        catch (...)
        {
            return nullptr;
        }
    };
}

inline napi_property_descriptor export_wrap_method_property(
    napi_env env,
    const char *utf8name,
    napi_callback method)
{
    napi_callback wrapped_method;
    void* data;
    wrap_callback(env, method, &wrapped_method, &data);
    return export_method_property(utf8name, wrapped_method, data);
}
#endif
