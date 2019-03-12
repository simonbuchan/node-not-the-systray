#pragma once

#include <functional>
#include <unordered_map>

#include <uv.h>
#include <Windows.h>

#include "napi.hh"

constexpr UINT WM_TRAYICON_CALLBACK = WM_USER + 123;

// Like std::unique_ptr, but doesn't assume pointers and takes the deleter as a
// value (auto-typed value template parameters is a c++17 feature, I believe).
template <typename T, auto deleter, T default_value = nullptr>
struct Unique
{
    T value = default_value;
    Unique() = default;
    Unique(T value) : value{ value } {}
    ~Unique()
    {
        deleter(value);
    }
    Unique(const Unique&) = delete;
    Unique& operator=(const Unique&) = delete;
    Unique(Unique&& other) { assign(other.release()); }
    Unique& operator=(Unique&& other)
    {
        assign(other.release());
        return *this;
    }

    operator T() const { return value; }
    Unique& operator =(T new_value)
    {
        assign(new_value);
        return *this;
    }

    T release() { return std::exchange(value, default_value); }

    void assign(T new_value)
    {
        if (value != new_value)
        {
            deleter(std::exchange(value, new_value));
        }
    }
};

using WndHandle = Unique<HWND, DestroyWindow>;
using MenuHandle = Unique<HMENU, DestroyMenu>;

struct MenuObject;
struct IconObject;

struct IconData
{
    napi_env env;
    uint16_t id;
    GUID guid;
    bool large_balloon_icon = false;
    NapiUnwrappedRef<IconObject> icon_ref;
    NapiUnwrappedRef<IconObject> notification_icon_ref;
    NapiAsyncCallback select_callback;
};

struct EnvData
{
    napi_env env = nullptr;
    napi_ref menu_constructor = nullptr;
    napi_ref icon_constructor = nullptr;
    WndHandle msg_hwnd;
    std::unordered_map<int32_t, IconData> icons;
    uv_idle_t message_pump_idle = { this };

    IconData* get_icon_data(int32_t icon_id);

    void start_message_pump();
    void stop_message_pump();
};

EnvData* get_env_data(napi_env env);

std::tuple<napi_status, EnvData*> create_env_data(napi_env env);
