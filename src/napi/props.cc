#include "napi/props.hh"

#include <vector>

napi_status napi_create_object(
    napi_env env, napi_value* result,
    std::initializer_list<napi_property_descriptor> descriptors) {
  NAPI_RETURN_IF_NOT_OK(napi_create_object(env, result));
  NAPI_RETURN_IF_NOT_OK(napi_define_properties(env, *result, descriptors));
  return napi_ok;
}

napi_status napi_create_object(
    napi_env env, napi_value* result,
    std::initializer_list<napi_output_property> properties) {
  NapiEscapableHandleScope scope;
  NAPI_RETURN_IF_NOT_OK(scope.open(env));

  std::vector<napi_property_descriptor> descriptors;
  descriptors.reserve(properties.size());
  for (const auto& prop : properties) {
    napi_value value;
    NAPI_RETURN_IF_NOT_OK(prop.output.get_value(env, &value));
    descriptors.push_back(
        napi_value_property(prop.utf8name, value, prop.attributes));
  }

  napi_value local;
  NAPI_RETURN_IF_NOT_OK(napi_create_object(env, &local));

  auto first = descriptors.data();
  auto last = first + descriptors.size();
  NAPI_RETURN_IF_NOT_OK(
      napi_define_properties(env, local, std::initializer_list(first, last)));

  return scope.escape(local, result);
}
