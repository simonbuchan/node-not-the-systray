#include "parse_guid.hh"

template <typename T>
bool parse_hex(std::string_view source, T* result) {
  T value = (T)0;
  for (auto c : source) {
    value <<= (T)4;
    if (isdigit(c))
      value |= (T)(c - '0');
    else if (isxdigit(c))
      value |= (T)(toupper(c) - 'A' + 10);
    else
      return false;
  }
  *result = value;
  return true;
}

template <typename T>
bool parse_hex(std::string_view source, T* result, int count) {
  int slice_size = source.size() / count;
  for (int i = 0; i < count; i++) {
    if (!parse_hex(source.substr(i * slice_size, slice_size), &result[i]))
      return false;
  }
  return true;
}

parse_guid_result parse_guid(std::string_view source, GUID* result) {
  // 32 bits +  16 bits +  16 bits +  16 bits +  48 bits = 128 bits
  // HEX{8} '-' HEX{4} '-' HEX{4} '-' HEX{4} '-' HEX{12} = 36 chars
  // 0       8  9      13  14     18  19     23  24
  if (source.size() != 36) return parse_guid_result::bad_size;
  if (source[8] != '-' || source[13] != '-' || source[18] != '-' ||
      source[23] != '-') {
    return parse_guid_result::bad_hyphens;
  }
  if (!parse_hex(source.substr(0, 8), &result->Data1) ||
      !parse_hex(source.substr(9, 4), &result->Data2) ||
      !parse_hex(source.substr(14, 4), &result->Data3) ||
      !parse_hex(source.substr(19, 4), &result->Data4[0], 2) ||
      !parse_hex(source.substr(24, 12), &result->Data4[2], 6)) {
    return parse_guid_result::bad_hex;
  }
  return parse_guid_result::ok;
}
