#pragma once

#include "napi-value.hh"

inline napi_status napi_define_properties(
    napi_env env,
    napi_value object,
    std::initializer_list<napi_property_descriptor> properties)
{
    return napi_define_properties(env, object, properties.size(), properties.begin());
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

inline napi_property_descriptor napi_value_property(
    const char *utf8name,
    napi_value value,
    napi_property_attributes attributes = napi_enumerable,
    void *data = nullptr)
{
    napi_property_descriptor desc = {};
    desc.utf8name = utf8name;
    desc.value = value;
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

// Can't be an overload, as it is otherwise higher priority than for std::*string_view!
inline napi_status napi_set_named_property_bool(
    napi_env env,
    napi_value object,
    const char* utf8name,
    bool native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_get_boolean(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}

inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    int32_t native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}

inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    uint32_t native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}

inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    double native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}

inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    std::string_view native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}

inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    std::u16string_view native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}

inline napi_status napi_set_named_property(
    napi_env env,
    napi_value object,
    const char* utf8name,
    std::wstring_view native_value)
{
    napi_value value = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create(env, native_value, &value));
    return napi_set_named_property(env, object, utf8name, value);
}
