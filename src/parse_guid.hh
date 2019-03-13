#pragma once

#include <guiddef.h>
#include <string_view>

enum class parse_guid_result {
  ok,
  bad_size,
  bad_hyphens,
  bad_hex,
};

parse_guid_result parse_guid(std::string_view source, GUID* result);
