#pragma once

#include "napi-core.hh"
#include "napi-ref.hh"

template <typename T>
struct NapiUnwrappedRef
{
    NapiRef ref;
    T *wrapped;
};

template <typename T>
struct NapiWrapped
{
    // Checks unwrap is safe by comparing a header pointer to this object
    inline static int type_object = 0;

    struct TypeWrapper
    {
        int* type_ptr = &type_object;
        T value;
    };

    static napi_status define_class(
        napi_env env,
        std::string_view name,
        napi_ref* constructor_ref)
    {
        napi_value constructor_value;
        NAPI_RETURN_IF_NOT_OK(napi_define_class(
            env,
            name.data(),
            name.size(),
            constructor,
            nullptr,
            0,
            nullptr,
            &constructor_value));
        return napi_create_reference(env, constructor_value, 1, constructor_ref);
    }

    static napi_value constructor(napi_env env, napi_callback_info info)
    {
        napi_value this_value = nullptr;
        NAPI_RETURN_NULL_IF_NOT_OK(napi_get_cb_info(env, info, nullptr, nullptr, &this_value, nullptr));
        auto ptr = std::make_unique<TypeWrapper>();
        NAPI_RETURN_NULL_IF_NOT_OK(napi_wrap(env, this_value, ptr.get(), finalize, nullptr, nullptr));
        ptr.release();
        return this_value;
    }

    static void finalize(napi_env env, void* finalize_data, void* finalize_hint)
    {
        delete (TypeWrapper*) finalize_data;
    }

    struct NewResult
    {
        napi_status status;
        napi_value wrapper;
        T* wrapped;
    };

    static NewResult new_instance(
        napi_env env,
        napi_ref constructor_ref,
        std::initializer_list<napi_value> args = {})
    {
        napi_value constructor = nullptr;
        NAPI_RETURN_INIT_IF_NOT_OK(napi_get_reference_value(
            env,
            constructor_ref,
            &constructor));
        return new_instance(
            env,
            constructor,
            args);
    }

    static NewResult new_instance(
        napi_env env,
        napi_value constructor,
        std::initializer_list<napi_value> args)
    {
        napi_value wrapper;
        NAPI_RETURN_INIT_IF_NOT_OK(napi_new_instance(
            env,
            constructor,
            args.size(),
            args.begin(),
            &wrapper));

        auto [status, wrapped] = try_unwrap(env, wrapper);
        return { status, wrapper, wrapped };
    }

    struct UnwrapResult
    {
        napi_status status;
        T* wrapped;
    };

    static UnwrapResult try_unwrap(napi_env env, napi_value value)
    {
        void* void_wrapped = nullptr;
        NAPI_RETURN_INIT_IF_NOT_OK(napi_unwrap(env, value, &void_wrapped));
        auto type_wrapped = (TypeWrapper*) void_wrapped;
        if (type_wrapped->type_ptr != &type_object)
        {
            return { napi_ok };
        }

        return { napi_ok, &type_wrapped->value };
    }

    using Ref = NapiUnwrappedRef<T>;

    static napi_status try_create_ref(napi_env env, napi_value value, Ref* result)
    {
        if (auto [status, wrapped] = try_unwrap(env, value); !wrapped)
        {
            result->wrapped = nullptr;
            return status;
        }
        else
        {
            NAPI_RETURN_IF_NOT_OK(result->ref.create(env, value));
            result->wrapped = wrapped;
            return napi_ok;
        }
    }
};

template <typename T>
napi_status napi_get_value(napi_env env, napi_value value, NapiUnwrappedRef<T>* result)
{
    NAPI_RETURN_IF_NOT_OK(NapiWrapped<T>::try_create_ref(env, value, result));
    if (!result->wrapped)
    {
        napi_throw_type_error(env, nullptr, "Invalid native object type");
        return napi_pending_exception;
    }

    return napi_ok;
}
