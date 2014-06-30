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

#include "mk_lua_cookie.h"
#include "mk_lua.h"
#include "time.h"

void mk_lua_cookie_init()
{
    int len;
    time_t expire = COOKIE_EXPIRE_TIME;
    struct tm *gmt;

    /* Init mk_ptr_t's */
    mk_api->pointer_set(&mk_lua_iov_none, "");
    mk_api->pointer_set(&mk_lua_cookie_crlf,      COOKIE_CRLF);
    mk_api->pointer_set(&mk_lua_cookie_equal,     COOKIE_EQUAL);
    mk_api->pointer_set(&mk_lua_cookie_set,       COOKIE_SET);
    mk_api->pointer_set(&mk_lua_cookie_expire,    COOKIE_EXPIRE);
    mk_api->pointer_set(&mk_lua_cookie_path,      COOKIE_PATH);
    mk_api->pointer_set(&mk_lua_cookie_semicolon, COOKIE_SEMICOLON);
    mk_api->pointer_set(&mk_lua_cookie_domain,    COOKIE_DOMAIN);
    mk_api->pointer_set(&mk_lua_cookie_secure,    COOKIE_SECURE);
    mk_api->pointer_set(&mk_lua_cookie_httponly,  COOKIE_HTTPONLY);
    /* Default expire value */
    mk_lua_cookie_expire_value.data = mk_api->mem_alloc_z(COOKIE_MAX_DATE_LEN);

    gmt = gmtime(&expire);
    len = strftime(mk_lua_cookie_expire_value.data,
                   COOKIE_MAX_DATE_LEN,
                   "%a, %d %b %Y %H:%M:%S GMT\r\n",
                   gmt);
    mk_lua_cookie_expire_value.len = len;
}


static int mk_lua_set_cookie(lua_State *L) {
    struct session_request *sr = mk_lua_get_session_request(L);

    const char *name, *value, *path = "", *domain = "";
    int secure = 0, expires = 0, httponly = 0;
    mk_ptr_t exp;
    char *header;
    
    lua_getfield(L, -1, "name");
    name = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    
    /* value */
    lua_getfield(L, -1, "value");
    value = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    
    /* expiry */
    lua_getfield(L, -1, "expiry");
    expires = luaL_optint(L, -1, 0);
    lua_pop(L, 1);
        
    /* secure */
    lua_getfield(L, -1, "secure");
        if (lua_isboolean(L, -1)) {
            secure = lua_toboolean(L, -1);
        }
    lua_pop(L, 1);
        
    /* httponly */
    lua_getfield(L, -1, "httponly");
        if (lua_isboolean(L, -1)) {
            httponly = lua_toboolean(L, -1);
        }
    lua_pop(L, 1);
        
    /* path */
    lua_getfield(L, -1, "path");
    path = luaL_optstring(L, -1, "");
    lua_pop(L, 1);
        
    /* domain */
    lua_getfield(L,-1, "domain");
    domain = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    /* We will build up the cookie line manually instead of using */
    /*    mk_api->header_send() in order to avoid memory allocations involved */
    
    if (!sr->headers._extra_rows) {
        sr->headers._extra_rows = mk_api->iov_create(MK_PLUGIN_HEADER_EXTRA_ROWS * 2, 0);
    }

    /* Add 'Set-Cookie: ' */
    mk_api->iov_add_entry(sr->headers._extra_rows, COOKIE_SET, sizeof(COOKIE_SET) -1,
                          mk_lua_iov_none, MK_IOV_NOT_FREE_BUF);

    /* Append 'KEY=' */
    mk_api->iov_add_entry(sr->headers._extra_rows, name, strlen(name),
                          mk_lua_cookie_equal, MK_IOV_NOT_FREE_BUF);

    /* Append 'VALUE; path=' */
    mk_api->iov_add_entry(sr->headers._extra_rows, value, strlen(value),
                          mk_lua_cookie_path, MK_IOV_NOT_FREE_BUF);

    mk_api->iov_add_entry(sr->headers._extra_rows, path, strlen(path),
                          mk_lua_iov_none, MK_IOV_NOT_FREE_BUF);
        
    if(httponly)
        mk_api->iov_add_entry(sr->headers._extra_rows, COOKIE_HTTPONLY, strlen(COOKIE_HTTPONLY),
                              mk_lua_iov_none, MK_IOV_NOT_FREE_BUF);

    if(secure)
        mk_api->iov_add_entry(sr->headers._extra_rows, COOKIE_SECURE, strlen(COOKIE_SECURE),
                              mk_lua_iov_none, MK_IOV_NOT_FREE_BUF);

    
    if (expires == COOKIE_EXPIRE_TIME) {
        mk_api->iov_add_entry(sr->headers._extra_rows, COOKIE_EXPIRE, strlen(COOKIE_EXPIRE),
                              mk_lua_iov_none, MK_IOV_NOT_FREE_BUF);
        
        mk_api->iov_add_entry(sr->headers._extra_rows,
                              mk_lua_cookie_expire_value.data,
                              mk_lua_cookie_expire_value.len,
                              mk_iov_none, MK_IOV_NOT_FREE_BUF);
    }
    
    /* If the expire time was set */
    else if (expires > 0) {
        exp.data = mk_api->mem_alloc(COOKIE_MAX_DATE_LEN);
        exp.len = mk_api->time_to_gmt(&exp.data, expires);
        mk_api->iov_add_entry(sr->headers._extra_rows, COOKIE_EXPIRE, strlen(COOKIE_EXPIRE),
                              mk_lua_iov_none, MK_IOV_NOT_FREE_BUF);
     
        mk_api->iov_add_entry(sr->headers._extra_rows,
                              exp.data, exp.len, mk_iov_none, MK_IOV_FREE_BUF);
    } else {
        mk_api->iov_add_entry(sr->headers._extra_rows, "", 0,
                              mk_lua_cookie_crlf, MK_IOV_NOT_FREE_BUF);
    }

    return 0;



}


static int mk_lua_get_cookies(lua_State *L) {

    int i;
    int len_key, len_val;
    int header_len = sizeof(COOKIE_HEADER) - 1;
    char *cookie;
    int length;
    struct headers_toc *toc;
    struct session_request *sr = mk_lua_get_session_request(L);

    lua_newtable(L);              /* lua table with cookies */
    

    /* Get headers table-of-content (TOC) */
    toc = &sr->headers_toc;
    for (i=0; i < toc->length; i++) {
        /* Compare header title */
        if (strncmp(toc->rows[i].init, COOKIE_HEADER, header_len) == 0) {
            break;
        }
    }

    if (i == toc->length) {
        return 1;
    }
    cookie = toc->rows[i].init + header_len + 1;
    length = toc->rows[i].end - cookie;
    char *cookie_current = cookie;
    int current_length = length;


    while(current_length > 0)
        {
            len_key = mk_api->str_search_n(cookie_current, "=", MK_STR_SENSITIVE, current_length);
            if(!len_key) break;
            len_val = mk_api->str_search_n(cookie_current + len_key + 1, ";", MK_STR_SENSITIVE, current_length - len_key -1);
            if(len_val == -1) len_val = current_length - len_key - 1;
            lua_pushlstring(L, cookie_current, len_key);
            lua_pushlstring(L, cookie_current + len_key + 1, len_val);
            cookie_current = cookie_current + len_key + 1 + len_val + 1;
            current_length -= (len_key + len_val + 2);
            lua_settable(L, -3);
        }
    
    return 1;
}

void mk_lua_inject_cookies(lua_State *L) {
    mk_lua_cookie_init();
    lua_newtable(L);
    lua_pushcfunction(L, mk_lua_get_cookies);
    lua_setfield(L, -2, "get_cookies");
    lua_pushcfunction(L, mk_lua_set_cookie);
    lua_setfield(L, -2, "set_cookie");
    
    lua_setfield(L, -2, "cookies");
    /* creates a cookies table on the table on the top of the stack when
       the fucntion is called */
}
  
