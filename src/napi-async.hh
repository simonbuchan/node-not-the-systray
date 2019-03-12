#pragma once

#include "napi-ref.hh"
#include "napi-value.hh"

struct NapiAsyncContext
{
    napi_env env = nullptr;
    napi_async_context context = nullptr;
    // Context resource object is recommended to match the init value
    napi_ref resource_object_ref = nullptr;

    NapiAsyncContext() = default;

    napi_status init(
        napi_env new_env,
        napi_value resource_name,
        napi_value resource_object = nullptr)
    {
        if (context)
        {
            NAPI_RETURN_IF_NOT_OK(napi_async_destroy(env, context));
        }
        if (resource_object_ref)
        {
            NAPI_RETURN_IF_NOT_OK(napi_reference_unref(env, resource_object_ref, nullptr));
        }
        if (!resource_object)
        {
            NAPI_RETURN_IF_NOT_OK(napi_create_object(new_env, &resource_object));
        }
        env = new_env;
        NAPI_RETURN_IF_NOT_OK(napi_create_reference(env, resource_object, 1, &resource_object_ref));
        return napi_async_init(env, resource_object, resource_name, &context);
    }

    napi_status init(
        napi_env new_env,
        std::string_view resource_name,
        napi_value resource_object = nullptr)
    {
        napi_value resource_name_value;
        NAPI_RETURN_IF_NOT_OK(napi_create(new_env, resource_name, &resource_name_value));
        return init(new_env, resource_name_value, resource_object);
    }

    ~NapiAsyncContext()
    {
        if (context) napi_async_destroy(env, context);
        if (resource_object_ref) napi_reference_unref(env, resource_object_ref, nullptr);
    }

    operator napi_async_context() const { return context; }

    NapiAsyncContext(NapiAsyncContext const&) = delete;
    NapiAsyncContext& operator=(NapiAsyncContext const&) = delete;
    NapiAsyncContext(NapiAsyncContext&& other)
        : env{ other.env }
        , context{ std::exchange(other.context, nullptr) }
        , resource_object_ref{ std::exchange(other.resource_object_ref, nullptr) }
    {
    }
    NapiAsyncContext& operator=(NapiAsyncContext&& other)
    {
        std::swap(env, other.env);
        std::swap(context, other.context);
        std::swap(resource_object_ref, other.resource_object_ref);
        return *this;
    }
};

struct NapiCallbackScope
{
    napi_env env = nullptr;
    napi_callback_scope scope = nullptr;

    NapiCallbackScope() = default;

    napi_status open(NapiAsyncContext const& context)
    {
        napi_value resource_object;
        NAPI_RETURN_IF_NOT_OK(napi_get_reference_value(
            context.env,
            context.resource_object_ref,
            &resource_object));
        return open(context.env, resource_object, context.context);
    }

    napi_status open(
        napi_env new_env,
        napi_value resource_object,
        napi_async_context context)
    {
        if (scope)
            NAPI_RETURN_IF_NOT_OK(napi_close_callback_scope(env, std::exchange(scope, nullptr)));
        env = new_env;
        return napi_open_callback_scope(env, resource_object, context, &scope);
    }

    ~NapiCallbackScope()
    {
        if (scope) napi_close_callback_scope(env, scope);
    }

    operator napi_callback_scope() const { return scope; }

    NapiCallbackScope(NapiCallbackScope const&) = delete;
    NapiCallbackScope& operator=(NapiCallbackScope const&) = delete;
    NapiCallbackScope(NapiCallbackScope&&) = delete;
    NapiCallbackScope& operator=(NapiCallbackScope&&) = delete;
};

struct NapiAsyncCallback : NapiRef
{
    NapiAsyncContext context;

    napi_status create(napi_env new_env, napi_value value, int refcount = 1)
    {
        NAPI_RETURN_IF_NOT_OK(context.init(new_env, "tray::NapiAsyncCallback"));
        NAPI_RETURN_IF_NOT_OK(NapiRef::create(new_env, value, refcount));
        return napi_ok;
    }

    template <typename... Args>
    napi_status operator()(napi_value* result, Args&&... args) const
    {
        NapiEscapableHandleScope handle_scope;
        NAPI_RETURN_IF_NOT_OK(handle_scope.open(env));

        // recv is checked for nullptr, but can be napi undefined.
        // See https://github.com/nodejs/node/issues/26342
        napi_value recv;
        NAPI_RETURN_IF_NOT_OK(napi_get_undefined(env, &recv));

        NapiCallbackScope callback_scope;
        NAPI_RETURN_IF_NOT_OK(callback_scope.open(context));

        napi_value func = nullptr;
        NAPI_RETURN_IF_NOT_OK(NapiRef::get(&func));

        napi_value arg_values[sizeof...(args)];
        napi_value result_value = nullptr;
        if (auto [status, status_arg_value] = napi_create_many(
                env,
                arg_values,
                std::forward<Args>(args)...);
            status != napi_ok)
        {
            napi_throw_last_error(env);
            return napi_pending_exception;
        }

        if (auto status = napi_call_function(
                env,
                recv,
                func,
                sizeof...(args),
                arg_values,
                &result_value);
            status != napi_ok)
        {
            napi_throw_last_error(env);
            return napi_pending_exception;
        }

        if (result)
        {
            NAPI_RETURN_IF_NOT_OK(handle_scope.escape(result_value, result));
        }

        return napi_ok;
    }
};
