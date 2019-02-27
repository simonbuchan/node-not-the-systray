#pragma once

#include "napi-value.hh"

#include <js_native_api.h>

struct NapiHandleScope
{
    napi_env env;
    napi_handle_scope scope;
    napi_status status;

    explicit NapiHandleScope(napi_env env)
        : env{ env }
        , status{ napi_open_handle_scope(env, &scope) }
    {
    }

    ~NapiHandleScope()
    {
        if (status == napi_ok) napi_close_handle_scope(env, scope);
    }

    operator napi_handle_scope() const { return scope; }

    NapiHandleScope(NapiHandleScope const&) = delete;
    NapiHandleScope& operator=(NapiHandleScope const&) = delete;
    NapiHandleScope(NapiHandleScope&&) = delete;
    NapiHandleScope& operator=(NapiHandleScope&&) = delete;
};

struct NapiCallbackScope
{
    napi_env env;
    napi_callback_scope scope;
    napi_status status;

    NapiCallbackScope(
        napi_env env,
        napi_value resource_object,
        napi_async_context context)
        : env{ env }
        , scope{}
        , status{ napi_open_callback_scope(env, resource_object, context, &scope) }
    {
    }

    ~NapiCallbackScope()
    {
        if (status == napi_ok) napi_close_callback_scope(env, scope);
    }

    operator napi_callback_scope() const { return scope; }

    NapiCallbackScope(NapiCallbackScope const&) = delete;
    NapiCallbackScope& operator=(NapiCallbackScope const&) = delete;
    NapiCallbackScope(NapiCallbackScope&&) = delete;
    NapiCallbackScope& operator=(NapiCallbackScope&&) = delete;
};

struct NapiAsyncContext
{
    napi_env env = nullptr;
    napi_async_context context = nullptr;

    NapiAsyncContext() = default;

    napi_status init(
        napi_env new_env,
        napi_value resource_name = nullptr,
        napi_value resource_object = nullptr)
    {
        if (context) napi_async_destroy(env, context);
        env = new_env;
        return napi_async_init(env, resource_object, resource_name, &context);
    }

    napi_status init(napi_env new_env, std::string_view resource_name, napi_value resource_object = nullptr)
    {
        napi_value resource_name_value;
        NAPI_RETURN_IF_NOT_OK(napi_create(new_env, resource_name, &resource_name_value));
        return init(new_env, resource_name_value, resource_object);
    }

    ~NapiAsyncContext()
    {
        if (context) napi_async_destroy(env, context);
    }

    operator napi_async_context() const { return context; }

    NapiAsyncContext(NapiAsyncContext const&) = delete;
    NapiAsyncContext& operator=(NapiAsyncContext const&) = delete;
    NapiAsyncContext(NapiAsyncContext&& other)
        : env{ other.env }
        , context{ std::exchange(other.context, nullptr) }
    {
    }
    NapiAsyncContext& operator=(NapiAsyncContext&& other)
    {
        std::swap(env, other.env);
        std::swap(context, other.context);
        return *this;
    }
};

struct NapiRef
{
    napi_env env = nullptr;
    napi_ref ref = nullptr;

    NapiRef() = default;
    NapiRef(napi_env env, napi_ref ref) : env{env}, ref{ref} {}
    ~NapiRef()
    {
        if (ref) napi_reference_unref(env, ref, nullptr);
    }

    NapiRef(NapiRef const& other)
        : env{other.env}
        , ref{other.ref}
    {
        if (ref) napi_reference_ref(env, ref, nullptr);
    }
    NapiRef(NapiRef&& other)
        : env{std::exchange(other.env, nullptr)}
        , ref{std::exchange(other.ref, nullptr)}
    {
    }

    NapiRef& operator=(NapiRef const& other)
    {
        assign_ref(other.env, other.ref);
        return *this;
    }

    NapiRef& operator=(NapiRef&& other)
    {
        assign_move(std::exchange(other.env, nullptr), std::exchange(other.ref, nullptr));
        return *this;
    }

    explicit operator bool() const { return ref != nullptr; }

    void clear()
    {
        assign_move(nullptr, nullptr);
    }

    void assign_move(napi_env new_env, napi_ref new_ref)
    {
        auto old_env = std::exchange(env, new_env);
        auto old_ref = std::exchange(ref, new_ref);
        if (old_ref) napi_reference_unref(old_env, old_ref, nullptr);
    }

    void assign_ref(napi_env new_env, napi_ref new_ref)
    {
        auto old_env = std::exchange(env, new_env);
        auto old_ref = std::exchange(ref, new_ref);
        if (new_ref) napi_reference_ref(new_env, new_ref, nullptr);
        if (old_ref) napi_reference_unref(old_env, old_ref, nullptr);
    }

    napi_status create(napi_env new_env, napi_value value, int refcount = 1)
    {
        napi_ref new_ref;
        NAPI_RETURN_IF_NOT_OK(napi_create_reference(new_env, value, refcount, &new_ref));
        assign_move(new_env, new_ref);
        return napi_ok;
    }

    napi_status get(napi_value* result) const
    {
        return napi_get_reference_value(env, ref, result);
    }
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
    void operator()(Args... args) const
    {
        NapiHandleScope handle_scope{env};
        NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, handle_scope.status);

        // context object is required and must be an object, but it doesn't need anything
        // specific. Similarly recv is checked for nullptr, but can be napi undefined.
        // I opened https://github.com/nodejs/node/issues/26342 about this, maybe I'm just dumb?
        napi_value object;
        NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, napi_create_object(env, &object));

        napi_value recv;
        NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, napi_get_undefined(env, &recv));

        NapiCallbackScope callback_scope{env, object, context};
        NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, callback_scope.status);

        napi_value func = nullptr;
        NAPI_THROW_RETURN_VOID_IF_NOT_OK(env, get(&func));

        napi_value arg_values[sizeof...(args)];
        napi_value result_value = nullptr;
        if (auto [status, status_arg_value] = napi_create_many(
                env,
                arg_values,
                std::forward<Args>(args)...);
            status != napi_ok)
        {
            napi_throw_last_error(env);
            return;
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
        }
    }
};
