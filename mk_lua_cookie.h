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
#ifndef _MK_LUA_COOKIE
#define _MK_LUA_COOKIE

#include "mk_lua.h"

#define COOKIE_CRLF          "\r\n"
#define COOKIE_EQUAL         "="
#define COOKIE_SET           "Set-Cookie: "
#define COOKIE_HEADER        "Cookie:"
#define COOKIE_EXPIRE        "; expires="
#define COOKIE_PATH          "; path=/"
#define COOKIE_HTTPONLY      "; HttpOnly"
#define COOKIE_SECURE        "; Secure"
#define COOKIE_SEMICOLON     "; "
#define COOKIE_DOMAIN        "; Domain="
#define COOKIE_DELETED       "deleted"
#define COOKIE_EXPIRE_TIME   337606980
#define COOKIE_MAX_DATE_LEN  32

mk_ptr_t mk_lua_cookie_crlf;
mk_ptr_t mk_lua_cookie_equal;
mk_ptr_t mk_lua_cookie_set  ;
mk_ptr_t mk_lua_cookie_expire;
mk_ptr_t mk_lua_cookie_expire_value;
mk_ptr_t mk_lua_cookie_path;
mk_ptr_t mk_lua_cookie_httponly;
mk_ptr_t mk_lua_cookie_secure;
mk_ptr_t mk_lua_cookie_domain;
mk_ptr_t mk_lua_cookie_semicolon;
mk_ptr_t mk_lua_iov_none;
void mk_lua_inject_cookies(lua_State *L);

#endif
