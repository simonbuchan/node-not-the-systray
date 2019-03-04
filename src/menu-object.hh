#pragma once

#include "napi-wrap.hh"
#include "data.hh"

struct MenuObject : NapiWrapped<MenuObject>
{
    MenuHandle menu;

    static napi_status define_class(EnvData* env_data, napi_value* constructor_value)
    {
        return NapiWrapped::define_class(
            env_data->env,
            "Menu",
            constructor_value,
            &env_data->menu_constructor);
    }

    static NewResult new_instance(EnvData* env_data, MenuHandle menu)
    {
        auto result = NapiWrapped::new_instance(env_data->env, env_data->menu_constructor);
        if (result.wrapped)
        {
            result.wrapped->menu = std::move(menu);
        }
        return result;
    }
};
