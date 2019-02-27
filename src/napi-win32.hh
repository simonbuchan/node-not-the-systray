#pragma once

#include "napi-core.hh"

typedef long HRESULT;

std::wstring format_win32_message(HRESULT code);

napi_status napi_create_win32_message(napi_env env, HRESULT code, napi_value* result);

napi_status napi_create_win32_error(napi_env env, const char* syscall, HRESULT code, napi_value* result);

napi_status napi_throw_win32_error(napi_env env, const char* syscall, HRESULT code);

napi_status napi_throw_win32_error(napi_env env, const char* syscall);
