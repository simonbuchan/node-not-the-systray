#include "napi/core.hh"

#include "napi/props.hh"

napi_value napi_get_and_clear_last_error(napi_env env) {
  const napi_extended_error_info* info = nullptr;
  NAPI_FATAL_IF_NOT_OK(napi_get_last_error_info(env, &info));

  // info gets invalidated by next napi call.
  napi_status status = napi_ok;
  napi_value message_value = nullptr;
  if (info) {
    status = info->error_code;
    if (info->error_message) {
      NAPI_FATAL_IF_NOT_OK(napi_create_string_utf8(
          env, info->error_message, NAPI_AUTO_LENGTH, &message_value));
    }
  }

  napi_value result = nullptr;
  bool is_pending = false;
  NAPI_FATAL_IF_NOT_OK(napi_is_exception_pending(env, &is_pending));
  if (is_pending) {
    NAPI_FATAL_IF_NOT_OK(napi_get_and_clear_last_exception(env, &result));
    return result;
  }

  if (!status) {
    NAPI_FATAL("No error info");
  }
  if (!message_value) {
    NAPI_FATAL("No error message");
  }

  switch (status) {
    case napi_ok:
    case napi_pending_exception:
      return nullptr;

    case napi_object_expected:
    case napi_string_expected:
    case napi_name_expected:
    case napi_function_expected:
    case napi_number_expected:
    case napi_boolean_expected:
    case napi_array_expected:
    case napi_bigint_expected:
      NAPI_FATAL_IF_NOT_OK(
          napi_create_type_error(env, nullptr, message_value, &result));
      return result;

    default:
      NAPI_FATAL_IF_NOT_OK(
          napi_create_error(env, nullptr, message_value, &result));
      return result;
  }
}

napi_status napi_throw_last_error(napi_env env) {
  return napi_throw(env, napi_get_and_clear_last_error(env));
}

napi_status napi_rethrow_with_location(napi_env env,
                                       std::string_view location) {
  std::string message;
  napi_value exception = napi_get_and_clear_last_error(env);
  bool is_error;
  NAPI_FATAL_IF_NOT_OK(napi_is_error(env, exception, &is_error));
  if (!is_error) {  // thrown value is not an error, coerce it to a string
    napi_value exception_string;
    NAPI_FATAL_IF_NOT_OK(
        napi_coerce_to_string(env, exception, &exception_string));
    NAPI_FATAL_IF_NOT_OK(napi_get_value(env, exception_string, &message));
    message.append(" (in "sv).append(location).append(")"sv);
    NAPI_FATAL_IF_NOT_OK(napi_throw_error(env, nullptr, message.c_str()));
  } else {  // else just append to .message
    NAPI_FATAL_IF_NOT_OK(
        napi_get_named_property(env, exception, "message", &message));
    message.append(" (in "sv).append(location).append(")"sv);
    NAPI_FATAL_IF_NOT_OK(
        napi_set_named_property(env, exception, "message", message));
    NAPI_FATAL_IF_NOT_OK(napi_throw(env, exception));
  }
  return napi_pending_exception;
}
