#pragma once

#include "napi/core.hh"

// We assume that wchar_t is UTF-16, since it should only have much usage on
// Windows.
static_assert(sizeof(wchar_t) == sizeof(char16_t));

// Looks weird, but it simplifies generic usages, like napi_get_many_values()

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  void* result) {
  return napi_ok;
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  napi_value* result) {
  *result = value;
  return napi_ok;
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  bool* result) {
  return napi_get_value_bool(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  int32_t* result) {
  return napi_get_value_int32(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  uint32_t* result) {
  return napi_get_value_uint32(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  double* result) {
  return napi_get_value_double(env, value, result);
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  std::string* result) {
  size_t size = 0;
  NAPI_RETURN_IF_NOT_OK(
      napi_get_value_string_utf8(env, value, nullptr, 0, &size));
  result->resize(size);
  return napi_get_value_string_utf8(env, value, (char*)result->data(),
                                    result->size() + 1, &size);
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  std::u16string* result) {
  size_t size = 0;
  NAPI_RETURN_IF_NOT_OK(
      napi_get_value_string_utf16(env, value, nullptr, 0, &size));
  result->resize(size);
  return napi_get_value_string_utf16(env, value, (char16_t*)result->data(),
                                     result->size() + 1, &size);
}

inline napi_status napi_get_value(napi_env env, napi_value value,
                                  std::wstring* result) {
  size_t size = 0;
  NAPI_RETURN_IF_NOT_OK(
      napi_get_value_string_utf16(env, value, nullptr, 0, &size));
  result->resize(size);
  return napi_get_value_string_utf16(env, value, (char16_t*)result->data(),
                                     result->size() + 1, &size);
}

template <typename T>
inline napi_status napi_get_value(napi_env env, napi_value value,
                                  std::optional<T>* result) {
  napi_valuetype type;
  NAPI_RETURN_IF_NOT_OK(napi_typeof(env, value, &type));
  if (type == napi_undefined) {
    result->reset();
    return napi_ok;
  }
  NAPI_RETURN_IF_NOT_OK(napi_get_value(env, value, &result->emplace()));
  return napi_ok;
}

template <typename... Ts>
std::tuple<napi_status, const napi_value*> napi_get_many_values(
    napi_env env, const napi_value* argv, Ts*... results) = delete;

inline std::tuple<napi_status, const napi_value*> napi_get_many_values(
    napi_env env, const napi_value* argv) {
  return {napi_ok, argv};
}

template <typename T, typename... Ts>
inline std::tuple<napi_status, const napi_value*> napi_get_many_values(
    napi_env env, const napi_value* argv, T* result, Ts*... results) {
  if (auto status = napi_get_value(env, *argv, result); status != napi_ok)
    return {status, argv};
  return napi_get_many_values(env, argv + 1, results...);
}

// For generic usage
inline napi_status napi_create(napi_env env, napi_value value,
                               napi_value* result) {
  *result = value;
  return napi_ok;
}

inline napi_status napi_create(napi_env env, bool value, napi_value* result) {
  return napi_get_boolean(env, value, result);
}

inline napi_status napi_create(napi_env env, int32_t value,
                               napi_value* result) {
  return napi_create_int32(env, value, result);
}

inline napi_status napi_create(napi_env env, uint32_t value,
                               napi_value* result) {
  return napi_create_uint32(env, value, result);
}

inline napi_status napi_create(napi_env env, double value, napi_value* result) {
  return napi_create_double(env, value, result);
}

// Need to explicitly overload for C strings and string literals so they don't
// decay to bool before invoking the string_view conversion.
inline napi_status napi_create(napi_env env, const char* value,
                               napi_value* result) {
  return napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, result);
}

template <int N>
inline napi_status napi_create(napi_env env, const char (&value)[N],
                               napi_value* result) {
  return napi_create_string_utf8(env, value, N - 1, result);
}

inline napi_status napi_create(napi_env env, std::string_view value,
                               napi_value* result) {
  return napi_create_string_utf8(env, value.data(), value.size(), result);
}

inline napi_status napi_create(napi_env env, const char16_t* value,
                               napi_value* result) {
  return napi_create_string_utf16(env, value, NAPI_AUTO_LENGTH, result);
}

template <int N>
inline napi_status napi_create(napi_env env, const char16_t (&value)[N],
                               napi_value* result) {
  return napi_create_string_utf16(env, value, N - 1, result);
}

inline napi_status napi_create(napi_env env, std::u16string_view value,
                               napi_value* result) {
  return napi_create_string_utf16(env, value.data(), value.size(), result);
}

template <int N>
inline napi_status napi_create(napi_env env, const wchar_t (&value)[N],
                               napi_value* result) {
  return napi_create_string_utf16(env, (const char16_t*)value, N - 1, result);
}

inline napi_status napi_create(napi_env env, const wchar_t* value,
                               napi_value* result) {
  return napi_create_string_utf16(env, (const char16_t*)value, NAPI_AUTO_LENGTH,
                                  result);
}

inline napi_status napi_create(napi_env env, std::wstring_view value,
                               napi_value* result) {
  return napi_create_string_utf16(env, (const char16_t*)value.data(),
                                  value.size(), result);
}

template <typename T>
inline napi_status napi_create(napi_env env, std::optional<T> value,
                               napi_value* result) {
  if (value) return napi_create(env, value.value(), result);
  return napi_get_null(env, result);
}

template <typename... Ts>
std::tuple<napi_status, napi_value*> napi_create_many(napi_env env,
                                                      napi_value* results,
                                                      Ts... values);

inline std::tuple<napi_status, napi_value*> napi_create_many(
    napi_env env, napi_value* results) {
  return {napi_ok, results};
}

template <typename T, typename... Ts>
inline std::tuple<napi_status, napi_value*> napi_create_many(
    napi_env env, napi_value* results, T value, Ts... values) {
  if (auto status = napi_create(env, value, results); status != napi_ok)
    return {status, results};
  return napi_create_many(env, results + 1, values...);
}

struct NapiHandleScope {
  napi_env env = nullptr;
  napi_handle_scope scope = nullptr;

  NapiHandleScope() = default;

  napi_status open(napi_env new_env) {
    if (scope)
      NAPI_RETURN_IF_NOT_OK(
          napi_close_handle_scope(env, std::exchange(scope, nullptr)));
    env = new_env;
    return napi_open_handle_scope(env, &scope);
  }

  ~NapiHandleScope() {
    if (scope) napi_close_handle_scope(env, scope);
  }

  operator napi_handle_scope() const { return scope; }

  NapiHandleScope(NapiHandleScope const&) = delete;
  NapiHandleScope& operator=(NapiHandleScope const&) = delete;
  NapiHandleScope(NapiHandleScope&&) = delete;
  NapiHandleScope& operator=(NapiHandleScope&&) = delete;
};

struct NapiEscapableHandleScope {
  napi_env env = nullptr;
  napi_escapable_handle_scope scope = nullptr;

  NapiEscapableHandleScope() = default;

  napi_status open(napi_env new_env) {
    if (scope)
      NAPI_RETURN_IF_NOT_OK(napi_close_escapable_handle_scope(
          env, std::exchange(scope, nullptr)));
    env = new_env;
    return napi_open_escapable_handle_scope(env, &scope);
  }

  napi_status escape(napi_value value, napi_value* result) {
    return napi_escape_handle(env, scope, value, result);
  }

  ~NapiEscapableHandleScope() {
    if (scope) napi_close_escapable_handle_scope(env, scope);
  }

  operator napi_escapable_handle_scope() const { return scope; }

  NapiEscapableHandleScope(NapiEscapableHandleScope const&) = delete;
  NapiEscapableHandleScope& operator=(NapiEscapableHandleScope const&) = delete;
  NapiEscapableHandleScope(NapiEscapableHandleScope&&) = delete;
  NapiEscapableHandleScope& operator=(NapiEscapableHandleScope&&) = delete;
};
