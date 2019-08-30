#pragma once

#include "napi/core.hh"

struct NapiRef {
  napi_env env = nullptr;
  napi_ref ref = nullptr;

  NapiRef() = default;
  NapiRef(napi_env env, napi_ref ref) : env{env}, ref{ref} {}
  ~NapiRef() {
    if (ref) napi_reference_unref(env, ref, nullptr);
  }

  NapiRef(NapiRef const& other) : env{other.env}, ref{other.ref} {
    if (ref) napi_reference_ref(env, ref, nullptr);
  }
  NapiRef(NapiRef&& other)
      : env{std::exchange(other.env, nullptr)},
        ref{std::exchange(other.ref, nullptr)} {}

  NapiRef& operator=(NapiRef const& other) {
    assign_ref(other.env, other.ref);
    return *this;
  }

  NapiRef& operator=(NapiRef&& other) {
    assign_move(std::exchange(other.env, nullptr),
                std::exchange(other.ref, nullptr));
    return *this;
  }

  explicit operator bool() const { return ref != nullptr; }

  void clear() { assign_move(nullptr, nullptr); }
  void release() {
    env = nullptr;
    ref = nullptr;
  }

  void assign_move(napi_env new_env, napi_ref new_ref) {
    auto old_env = std::exchange(env, new_env);
    auto old_ref = std::exchange(ref, new_ref);
    if (old_ref) napi_reference_unref(old_env, old_ref, nullptr);
  }

  void assign_ref(napi_env new_env, napi_ref new_ref) {
    auto old_env = std::exchange(env, new_env);
    auto old_ref = std::exchange(ref, new_ref);
    if (new_ref) napi_reference_ref(new_env, new_ref, nullptr);
    if (old_ref) napi_reference_unref(old_env, old_ref, nullptr);
  }

  napi_status create(napi_env new_env, napi_value value, int refcount = 1) {
    napi_ref new_ref;
    NAPI_RETURN_IF_NOT_OK(
        napi_create_reference(new_env, value, refcount, &new_ref));
    assign_move(new_env, new_ref);
    return napi_ok;
  }

  napi_status get(napi_value* result) const {
    return napi_get_reference_value(env, ref, result);
  }
};