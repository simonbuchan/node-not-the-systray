#pragma once

#include "napi-core.hh"

static_assert(sizeof(wchar_t) == sizeof(char16_t));

// Looks weird, but it simplifies generic usages, like napi_get_many_values()

inline napi_status napi_get_value(napi_env env, napi_value value, void* result)
{
    return napi_ok;
}

inline napi_status napi_get_value(napi_env env, napi_value value, napi_value* result)
{
    *result = value;
    return napi_ok;
}

inline napi_status napi_get_value(napi_env env, napi_value value, bool* result)
{
    return napi_get_value_bool(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value, int32_t* result)
{
    return napi_get_value_int32(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value, uint32_t* result)
{
    return napi_get_value_uint32(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value, double* result)
{
    return napi_get_value_double(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value, std::string* result)
{
    size_t size = 0;
    NAPI_RETURN_IF_NOT_OK(napi_get_value_string_utf8(env, value, nullptr, 0, &size));
    result->resize(size);
    return napi_get_value_string_utf8(env, value, (char*) result->data(), result->size() + 1, &size);
}

inline napi_status napi_get_value(napi_env env, napi_value value, std::u16string* result)
{
    size_t size = 0;
    NAPI_RETURN_IF_NOT_OK(napi_get_value_string_utf16(env, value, nullptr, 0, &size));
    result->resize(size);
    return napi_get_value_string_utf16(env, value, (char16_t*) result->data(), result->size() + 1, &size);
}

inline napi_status napi_get_value(napi_env env, napi_value value, std::wstring* result)
{
    size_t size = 0;
    NAPI_RETURN_IF_NOT_OK(napi_get_value_string_utf16(env, value, nullptr, 0, &size));
    result->resize(size);
    return napi_get_value_string_utf16(env, value, (char16_t*) result->data(), result->size() + 1, &size);
}

template <typename T>
inline napi_status napi_get_value(napi_env env, napi_value value, std::optional<T>* result)
{
    napi_valuetype type;
    NAPI_RETURN_IF_NOT_OK(napi_typeof(env, value, &type));
    if (type == napi_undefined)
    {
        result->reset();
        return napi_ok;
    }
    NAPI_RETURN_IF_NOT_OK(napi_get_value(env, value, &result->emplace()));
    return napi_ok;
}

template <typename... Ts>
std::tuple<napi_status, const napi_value*> napi_get_many_values(
    napi_env env,
    const napi_value* argv,
    Ts*... results) = delete;

inline std::tuple<napi_status, const napi_value*> napi_get_many_values(
    napi_env env,
    const napi_value* argv)
{
    return {napi_ok, argv};
}

template <typename T, typename... Ts>
inline std::tuple<napi_status, const napi_value*> napi_get_many_values(
    napi_env env,
    const napi_value* argv,
    T* result,
    Ts*... results)
{
    if (auto status = napi_get_value(env, *argv, result); status != napi_ok)
        return {status, argv};
    return napi_get_many_values(env, argv + 1, results...);
}

// A hack to ensure bool isn't selected over e.g. std::string_view for char*,
// which also means we should use forwarding to avoid copying std::strings.
// You can still just add overloads for napi_create() if you won't hit this
// dumb edge case, but napi_create_() will work too if you will.
template <typename T>
inline napi_status napi_create(napi_env env, T&& value, napi_value* result)
{
    return napi_create_(env, std::forward<T>(value), result);
}

template <>
inline napi_status napi_create(napi_env env, bool&& value, napi_value* result)
{
    return napi_get_boolean(env, value, result);
}

inline napi_status napi_create_(napi_env env, int32_t value, napi_value* result)
{
    return napi_create_int32(env, value, result);
}

inline napi_status napi_create_(napi_env env, uint32_t value, napi_value* result)
{
    return napi_create_uint32(env, value, result);
}

inline napi_status napi_create_(napi_env env, double value, napi_value* result)
{
    return napi_create_double(env, value, result);
}

inline napi_status napi_create_(napi_env env, std::string_view value, napi_value* result)
{
    return napi_create_string_utf8(env, value.data(), value.size(), result);
}

inline napi_status napi_create_(napi_env env, std::u16string_view value, napi_value* result)
{
    return napi_create_string_utf16(env, value.data(), value.size(), result);
}

inline napi_status napi_create_(napi_env env, std::wstring_view value, napi_value* result)
{
    return napi_create_string_utf16(env, (const char16_t*) value.data(), value.size(), result);
}

template <typename... Ts>
std::tuple<napi_status, napi_value*> napi_create_many(
    napi_env env,
    napi_value* results,
    Ts... values);

inline std::tuple<napi_status, napi_value*> napi_create_many(
    napi_env env,
    napi_value* results)
{
    return {napi_ok, results};
}

template <typename T, typename... Ts>
inline std::tuple<napi_status, napi_value*> napi_create_many(
    napi_env env,
    napi_value* results,
    T value,
    Ts... values)
{
    if (auto status = napi_create(env, value, results); status != napi_ok)
        return {status, results};
    return napi_create_many(env, results + 1, values...);
}
