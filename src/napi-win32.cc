#include "napi-win32.hh"

#include "napi-props.hh"
#include "napi-value.hh"

#include <Windows.h>

std::wstring format_win32_message(HRESULT code)
{
    LPWSTR message_local = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        code,
        LANG_USER_DEFAULT,
        (LPWSTR)&message_local,
        0,
        nullptr);
    if (!message_local)
        return L"Unknown error."s;

    // Trim ending \r\n
    auto len = wcslen(message_local);
    while (len && iswspace(message_local[len - 1]))
        --len;
    std::wstring result(message_local, len);
    LocalFree(message_local);
    return result;
}

napi_status napi_create_win32_message(napi_env env, HRESULT code, napi_value* result)
{
    return napi_create(env, format_win32_message(code), result);
}

napi_status napi_create_win32_error(napi_env env, const char* syscall, HRESULT code, napi_value* result)
{
    napi_value message, error;
    *result = nullptr;
    NAPI_RETURN_IF_NOT_OK(napi_create_win32_message(env, code, &message));
    NAPI_RETURN_IF_NOT_OK(napi_create_error(env, nullptr, message, &error));
    NAPI_RETURN_IF_NOT_OK(napi_set_named_property(env, error, "name", "Win32Error"));
    NAPI_RETURN_IF_NOT_OK(napi_set_named_property(env, error, "syscall", syscall));
    NAPI_RETURN_IF_NOT_OK(napi_set_named_property(env, error, "errno", code));
    *result = error;
    return napi_ok;
}

napi_status napi_throw_win32_error(napi_env env, const char* syscall, HRESULT code)
{
    napi_value error;
    NAPI_RETURN_IF_NOT_OK(napi_create_win32_error(env, syscall, code, &error));
    return napi_throw(env, error);
}

napi_status napi_throw_win32_error(napi_env env, const char* syscall)
{
    return napi_throw_win32_error(env, syscall, GetLastError());
}
