#pragma once

#include <bitset>
#include <functional>
#include <unordered_map>

#include <uv.h>
// #include <Windows.h>

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
using IconHandle = Unique<HICON, DestroyIcon>;

struct IconData
{
    napi_env env;
    uint16_t id;
    GUID guid;
    HMENU menu = nullptr;
    HICON icon = nullptr;
    HICON balloon_icon = nullptr;
    bool large_balloon_icon = false;
    // We don't want to DestroyIcon() shared icons.
    IconHandle custom_icon;
    IconHandle custom_balloon_icon;
    NapiAsyncCallback select_callback;
};

struct EnvData
{
    napi_env env;
    WndHandle msg_hwnd;
    std::unordered_map<int32_t, IconData> icons;
    uv_idle_t message_pump_idle = { this };

    static void message_pump_idle_cb(uv_idle_t* idle)
    {
        auto data = (EnvData*) idle->data;

        MSG msg;
        while (PeekMessage(&msg, data->msg_hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void start_message_pump()
    {
        uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
    }

    void stop_message_pump()
    {
        uv_idle_start(&message_pump_idle, &message_pump_idle_cb);
    }
};

EnvData* get_env_data(napi_env env);

IconData* get_icon_data(napi_env env, int32_t icon_id);

EnvData* get_or_create_env_data(napi_env env);
