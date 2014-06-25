/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Monkey HTTP Server
 *  ==================
 *  Copyright 2001-2014 Monkey Software LLC <eduardo@monkey.io>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include "mk_lua.h"

/* parse url encoded strings into two tables and pushes them into the
   stack */
void mk_lua_urlencoded_to_table(lua_State *L, char *qs)
{
    char *key;
    char *value;
    char *strtok_state;
    int t;
  
    key = strtok_r(qs, "&", &strtok_state);
    while (key) {
        value = strchr(key, '=');
        if (value) {
            *value = '\0';      /* Split the string in two */
            value++;            /* Skip passed the = */
        }
        else {
            value = "1";
        }
        /*
          store the key and values into tables
        */

        lua_getfield(L, -1, key);   /* [VALUE, table<s,t>, table<s,s>] */
        /* borrowed from apache mod_lua */
        t = lua_type(L, -1);
        switch (t) {
        case LUA_TNIL:
        case LUA_TNONE:{
            lua_pop(L, 1);      /* [table<s,t>, table<s,s>] */
            lua_newtable(L);    /* [array, table<s,t>, table<s,s>] */
            lua_pushnumber(L, 1);       /* [1, array, table<s,t>, table<s,s>] */
            lua_pushstring(L, value);   /* [string, 1, array, table<s,t>, table<s,s>] */
            lua_settable(L, -3);        /* [array, table<s,t>, table<s,s>]  */
            lua_setfield(L, -2, key);   /* [table<s,t>, table<s,s>] */
            break;
        }
        case LUA_TTABLE:{
            /* [array, table<s,t>, table<s,s>] */
            int size = lua_rawlen(L, -1);
            lua_pushnumber(L, size + 1);        /* [#, array, table<s,t>, table<s,s>] */
            lua_pushstring(L, value);   /* [string, #, array, table<s,t>, table<s,s>] */
            lua_settable(L, -3);        /* [array, table<s,t>, table<s,s>] */
            lua_setfield(L, -2, key);   /* [table<s,t>, table<s,s>] */
            break;
        }
        }

        /* L is [table<s,t>, table<s,s>] */
        /* build simple */
        lua_getfield(L, -2, key);   /* [VALUE, table<s,s>, table<s,t>] */
        if (lua_isnoneornil(L, -1)) {       /* only set if not already set */
            lua_pop(L, 1);          /* [table<s,s>, table<s,t>]] */
            lua_pushstring(L, value);       /* [string, table<s,s>, table<s,t>] */
            lua_setfield(L, -3, key);       /* [table<s,s>, table<s,t>]  */
        }
        else {
            lua_pop(L, 1);
        }

        key = strtok_r(NULL, "&", &strtok_state);
    }    

}

