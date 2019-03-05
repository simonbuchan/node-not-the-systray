#pragma once

#include "napi-wrap.hh"
#include "data.hh"

struct MenuObject : NapiWrapped<MenuObject>
{
    MenuHandle menu;

    static napi_status define_class(EnvData* env_data, napi_value* constructor_value);
    static NewResult new_instance(EnvData* env_data, MenuHandle menu);
};
