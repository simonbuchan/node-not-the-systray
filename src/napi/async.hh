#pragma once

#include "napi/ref.hh"
#include "napi/value.hh"

#include <functional>

struct NapiAsyncContext {
  napi_env env = nullptr;
  napi_async_context context = nullptr;
  // Context resource object is recommended to match the init value
  napi_ref resource_object_ref = nullptr;

  NapiAsyncContext() = default;

  napi_status init(napi_env new_env, napi_value resource_name,
                   napi_value resource_object = nullptr) {
    if (context) {
      NAPI_RETURN_IF_NOT_OK(napi_async_destroy(env, context));
    }
    if (resource_object_ref) {
      NAPI_RETURN_IF_NOT_OK(
          napi_reference_unref(env, resource_object_ref, nullptr));
    }
    if (!resource_object) {
      NAPI_RETURN_IF_NOT_OK(napi_create_object(new_env, &resource_object));
    }
    env = new_env;
    NAPI_RETURN_IF_NOT_OK(
        napi_create_reference(env, resource_object, 1, &resource_object_ref));
    return napi_async_init(env, resource_object, resource_name, &context);
  }

  napi_status init(napi_env new_env, std::string_view resource_name,
                   napi_value resource_object = nullptr) {
    napi_value resource_name_value;
    NAPI_RETURN_IF_NOT_OK(
        napi_create(new_env, resource_name, &resource_name_value));
    return init(new_env, resource_name_value, resource_object);
  }

  ~NapiAsyncContext() {
    if (context) napi_async_destroy(env, context);
    if (resource_object_ref)
      napi_reference_unref(env, resource_object_ref, nullptr);
  }

  operator napi_async_context() const { return context; }

  NapiAsyncContext(NapiAsyncContext const&) = delete;
  NapiAsyncContext& operator=(NapiAsyncContext const&) = delete;
  NapiAsyncContext(NapiAsyncContext&& other)
      : env{other.env},
        context{std::exchange(other.context, nullptr)},
        resource_object_ref{std::exchange(other.resource_object_ref, nullptr)} {
  }
  NapiAsyncContext& operator=(NapiAsyncContext&& other) {
    std::swap(env, other.env);
    std::swap(context, other.context);
    std::swap(resource_object_ref, other.resource_object_ref);
    return *this;
  }
};

struct NapiCallbackScope {
  napi_env env = nullptr;
  napi_callback_scope scope = nullptr;

  NapiCallbackScope() = default;

  napi_status open(NapiAsyncContext const& context) {
    napi_value resource_object;
    NAPI_RETURN_IF_NOT_OK(napi_get_reference_value(
        context.env, context.resource_object_ref, &resource_object));
    return open(context.env, resource_object, context.context);
  }

  napi_status open(napi_env new_env, napi_value resource_object,
                   napi_async_context context) {
    if (scope)
      NAPI_RETURN_IF_NOT_OK(
          napi_close_callback_scope(env, std::exchange(scope, nullptr)));
    env = new_env;
    return napi_open_callback_scope(env, resource_object, context, &scope);
  }

  ~NapiCallbackScope() {
    if (scope) napi_close_callback_scope(env, scope);
  }

  operator napi_callback_scope() const { return scope; }

  NapiCallbackScope(NapiCallbackScope const&) = delete;
  NapiCallbackScope& operator=(NapiCallbackScope const&) = delete;
  NapiCallbackScope(NapiCallbackScope&&) = delete;
  NapiCallbackScope& operator=(NapiCallbackScope&&) = delete;
};

struct NapiAsyncCallback : NapiRef {
  NapiAsyncContext context;

  napi_status create(napi_env new_env, napi_value value, int refcount = 1) {
    NAPI_RETURN_IF_NOT_OK(context.init(new_env, "tray::NapiAsyncCallback"));
    NAPI_RETURN_IF_NOT_OK(NapiRef::create(new_env, value, refcount));
    return napi_ok;
  }

  napi_value operator()(napi_value recv,
                        std::initializer_list<napi_value> args) const {
    napi_value result = nullptr;

    NapiCallbackScope callback_scope;
    NAPI_RETURN_NULL_IF_NOT_OK(callback_scope.open(context));

    napi_value func = nullptr;
    NAPI_RETURN_NULL_IF_NOT_OK(NapiRef::get(&func));

    if (auto status = napi_call_function(env, recv, func, args.size(),
                                         args.begin(), &result);
        status != napi_ok) {
      napi_throw_last_error(env);
    }

    return result;
  }
};

struct NapiThreadsafeFunctionBase {
  struct Value {
    napi_threadsafe_function value = nullptr;

    Value() = default;
    Value(napi_threadsafe_function value) : value{value} {}
    operator napi_threadsafe_function() const { return value; }

    napi_status create(napi_env env, napi_value func,
                       napi_threadsafe_function_call_js call_js,
                       napi_value resource_object, napi_value resource_name,
                       void* context = nullptr, int max_queue_size = 0) {
      return napi_create_threadsafe_function(
          env, func, resource_object, resource_name, max_queue_size, 1, nullptr,
          nullptr, context, call_js, &value);
    }

    napi_status ref(napi_env env) const {
      return napi_ref_threadsafe_function(env, value);
    }

    napi_status unref(napi_env env) const {
      return napi_unref_threadsafe_function(env, value);
    }

    napi_status acquire() const {
      return napi_acquire_threadsafe_function(value);
    }

    napi_status release(napi_threadsafe_function_release_mode mode) const {
      return napi_release_threadsafe_function(value, mode);
    }

    napi_status call(void* data,
                     napi_threadsafe_function_call_mode mode) const {
      return napi_call_threadsafe_function(value, data, mode);
    }

    napi_status call_nonblocking(void* data) const {
      return napi_call_threadsafe_function(value, data, napi_tsfn_nonblocking);
    }

    napi_status call_blocking(void* data) const {
      return napi_call_threadsafe_function(value, data, napi_tsfn_blocking);
    }
  };

  struct Guard {
    Value value;
    explicit Guard(const NapiThreadsafeFunctionBase& fn) : value{fn.value} {
      NAPI_FATAL_IF_NOT_OK(value.acquire());
    }
    ~Guard() { NAPI_FATAL_IF_NOT_OK(value.release(napi_tsfn_release)); }
  };

  Value value;

  NapiThreadsafeFunctionBase() = default;
  ~NapiThreadsafeFunctionBase() { NAPI_FATAL_IF_NOT_OK(clear()); }

  NapiThreadsafeFunctionBase(const NapiThreadsafeFunctionBase& other) = delete;
  NapiThreadsafeFunctionBase& operator=(
      const NapiThreadsafeFunctionBase& other) = delete;

  NapiThreadsafeFunctionBase(NapiThreadsafeFunctionBase&& other) {
    std::exchange(value.value, other.value);
  }
  NapiThreadsafeFunctionBase& operator=(NapiThreadsafeFunctionBase&& other) {
    std::exchange(value.value, other.value);
    return *this;
  }

  napi_status clear(
      napi_threadsafe_function_release_mode mode = napi_tsfn_abort) {
    if (value) {
      NAPI_RETURN_IF_NOT_OK(value.release(mode));
      value = nullptr;
    }
    return napi_ok;
  }

  napi_status create(napi_env env, napi_value func,
                     napi_threadsafe_function_call_js call_js = nullptr,
                     void* context = nullptr, int max_queue_size = 0) {
    napi_value resource_object;
    NAPI_RETURN_IF_NOT_OK(napi_create_object(env, &resource_object));
    napi_value resource_name;
    NAPI_RETURN_IF_NOT_OK(
        napi_create(env, "tray::NapiAsyncFunction", &resource_name));
    NAPI_RETURN_IF_NOT_OK(clear());

    return value.create(env, func, call_js, resource_object, resource_name,
                        context, max_queue_size);
  }
};

struct NapiThreadsafeFunction : NapiThreadsafeFunctionBase {
  using CallData = std::function<void(napi_env, napi_value)>;

  napi_status create(napi_env env, int max_queue_size = 0) {
    return NapiThreadsafeFunctionBase::create(env, nullptr, call_js, nullptr,
                                              max_queue_size);
  }

  napi_status create(napi_env env, napi_value func, int max_queue_size = 0) {
    return NapiThreadsafeFunctionBase::create(env, func, call_js, nullptr,
                                              max_queue_size);
  }

  template <typename Body>
  napi_status nonblocking(Body&& body) const {
    auto data = std::make_unique<CallData>(std::move(body));
    NAPI_RETURN_IF_NOT_OK(value.call_nonblocking(data.get()));
    data.release();
    return napi_ok;
  }

  template <typename Body>
  napi_status blocking(Body&& body) const {
    auto data = std::make_unique<CallData>(std::move(body));
    NAPI_RETURN_IF_NOT_OK(value.call_blocking(data.get()));
    data.release();
    return napi_ok;
  }

  static void call_js(napi_env env, napi_value func, void* context,
                      void* data) {
    auto fn_ptr = std::unique_ptr<CallData>(static_cast<CallData*>(data));
    (*fn_ptr)(env, func);
  }
};

struct NapiDeferred {
  napi_env env = nullptr;
  napi_value promise = nullptr;
  napi_deferred deferred = nullptr;

  napi_status create(napi_env new_env) {
    env = new_env;
    return napi_create_promise(new_env, &deferred, &promise);
  }

  napi_status resolve(napi_value value) {
    return napi_resolve_deferred(env, deferred, value);
  }

  napi_status reject(napi_value value) {
    return napi_reject_deferred(env, deferred, value);
  }
};
