#pragma once

#include "napi-value.hh"

inline napi_status napi_define_properties(
    napi_env env,
    napi_value object,
    std::initializer_list<napi_property_descriptor> properties)
{
    return napi_define_properties(env, object, properties.size(), properties.begin());
}

inline napi_property_descriptor napi_value_property(
    const char *utf8name,
    napi_value value,
    napi_property_attributes attributes = napi_enumerable)
{
    napi_property_descriptor desc = {};
    desc.utf8name = utf8name;
    desc.value = value;
    desc.attributes = attributes;
    return desc;
}

inline napi_property_descriptor napi_getter_property(
    const char *utf8name,
    napi_callback getter,
    napi_property_attributes attributes = napi_enumerable,
    void *data = nullptr)
{
    napi_property_descriptor desc = {};
    desc.utf8name = utf8name;
    desc.getter = getter;
    desc.attributes = attributes;
    desc.data = data;
    return desc;
}

inline napi_property_descriptor napi_method_property(
    const char *utf8name,
    napi_callback method,
    napi_property_attributes attributes = napi_enumerable,
    void *data = nullptr)
{
    napi_property_descriptor desc = {};
    desc.utf8name = utf8name;
    desc.method = method;
    desc.attributes = attributes;
    desc.data = data;
    return desc;
}

template <typename T>
inline napi_status napi_get_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    T* result)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_get_named_property(env, object, utf8name, &value));
    return napi_get_value(env, value, result);
}

template <typename T>
inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    T&& native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, std::forward<T>(native_value), &value));
    return napi_set_named_property(env, object, utf8name, value);
}

struct napi_erased_output
{
    const void *erased_value;
    napi_status (*erased_get_value)(napi_env env, const void *native_data, napi_value *result);
    napi_status get_value(napi_env env, napi_value *result) const
    {
        return erased_get_value(env, erased_value, result);
    }
};

template <typename T>
napi_erased_output napi_output(const T& value)
{
    return {
        &value,
        [](napi_env env, const void *erased_value, napi_value *result) {
            auto value_ptr = (const T *)erased_value;
            return napi_create(env, *value_ptr, result);
        }};
}

struct napi_output_property
{
    const char* utf8name;
    napi_erased_output output;
    napi_property_attributes attributes;

    template <typename T>
    napi_output_property(
        const char* utf8name,
        const T& value,
        napi_property_attributes attributes = napi_enumerable)
        : utf8name{utf8name}
        , output{napi_output(value)}
        , attributes{attributes}
    {
    }
};

napi_status napi_create_object(
    napi_env env,
    napi_value* result,
    std::initializer_list<napi_output_property> properties);
