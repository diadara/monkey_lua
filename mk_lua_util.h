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
#ifndef _MK_LUA_UTIL
#define _MK_LUA_UTIL

#include "mk_lua.h"

void mk_lua_urlencoded_to_table(lua_State *L, char *qs);

static inline struct client_session* mk_lua_get_client_session(lua_State *L)
  {
    lua_getglobal(L, "__mk_lua_cs");
    struct client_session *cs = lua_touserdata(L, -1);
    if(!cs)
        printf("returned null!");
    lua_pop(L, 1);
    return cs;
  }


static inline struct session_request* mk_lua_get_session_request(lua_State *L)
  {
    lua_getglobal(L, "__mk_lua_sr");
    struct session_request *sr = lua_touserdata(L, -1);
    if(!sr)
        printf("returned null!");
    lua_pop(L, 1);
    return sr;
  }


static inline struct lua_request* mk_lua_get_lua_request(lua_State *L)
  {
    lua_getglobal(L, "__mk_lua_req");
    struct lua_request *r = lua_touserdata(L, -1);
    if(!r)
        printf("returned null!");
    lua_pop(L, 1);
    return r;
  }

#endif
