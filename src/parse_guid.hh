#pragma once

#include <string_view>
#include <guiddef.h>

enum class parse_guid_result
{
    ok,
    bad_size,
    bad_hyphens,
    bad_hex,
};

parse_guid_result parse_guid(std::string_view source, GUID* result);
