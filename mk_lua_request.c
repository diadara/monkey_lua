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
#include "string.h"
#include "ctype.h"
#include "dbg.h"

void mk_lua_multipart_to_table(lua_State *L,
                               char *boundary,
                               char *data)
{
    char *value, *key, *filename;
    char *start = data, *end = 0, *crlf = 0;
    unsigned long int vlen = 0;
    unsigned int len = 0;
    len = strlen(boundary);
    int t;
    start = strstr(data, boundary); /* start points to the first
                                       boundary */
    while(start) {
        end = strstr((char *) (start + 1), boundary);
        /* find the next boundary and store it in end*/
        if (end == NULL) break;            /* if no boundary, we have parsed
                                              form data */
        crlf = strstr((char *) start, "\r\n\r\n");
        if (!crlf) break;           /* there's no data */
        key = (char *) mk_api->mem_alloc_z(256);
        filename = (char *) mk_api->mem_alloc_z(256);
        vlen = end - crlf - 8;
        value = (char *) mk_api->mem_alloc_z(vlen+1);
        memcpy(value, crlf + 4, vlen);
        sscanf(start + len + 2,
               "Content-Disposition: form-data; name=\"%255[^\"]\"; filename=\"%255[^\"]\"",
               key, filename);


        if (strlen(key)) {
            lua_getfield(L, -1, key);   /* [VALUE, table<s,t>, table<s,s>] */
            /* borrowed from apache mod_lua */
            t = lua_type(L, -1);
            switch (t) {
            case LUA_TNIL:
            case LUA_TNONE:{
                lua_pop(L, 1);      /* [table<s,t>, table<s,s>] */
                lua_newtable(L);    /* [array, table<s,t>, table<s,s>] */
                lua_pushnumber(L, 1);       /* [1, array, table<s,t>, table<s,s>] */
                lua_pushlstring(L, value, vlen);   /* [string, 1, array, table<s,t>, table<s,s>] */
                lua_settable(L, -3);        /* [array, table<s,t>, table<s,s>]  */
                lua_setfield(L, -2, key);   /* [table<s,t>, table<s,s>] */
                break;
            }
            case LUA_TTABLE:{
                /* [array, table<s,t>, table<s,s>] */
                int size = lua_rawlen(L, -1);
                lua_pushnumber(L, size + 1);        /* [#, array, table<s,t>, table<s,s>] */
                lua_pushlstring(L, value, vlen);   /* [string, #, array, table<s,t>, table<s,s>] */
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
        }
        mk_api->mem_free(value);
        mk_api->mem_free(key);
        mk_api->mem_free(filename);
        start = end;
    }
}


static int  mk_lua_query_to_table(lua_State *L)
{
    lua_getglobal(L, "mk");
    lua_getfield(L, -1, "request");
    lua_getfield(L, -1, "query_string");
    char *qs = lua_tostring(L,-1);

    lua_newtable(L);
    lua_newtable(L);              /* [table, table] */

    mk_lua_urlencoded_to_table(L, qs);
    return 2;
}


static int mk_lua_data_to_table(lua_State *L)
{
    struct session_request *sr = mk_lua_get_session_request(L);
    char *content_type = mk_api->pointer_to_buf(sr->content_type);
    char *data = mk_api->pointer_to_buf(sr->data);
    char *multipart = mk_api->mem_alloc(256);
    lua_newtable(L);
    lua_newtable(L);
  
    if (strcmp(content_type, FORM_URLENCODED) == 0) {
        mk_lua_urlencoded_to_table(L, data);
        return 2;
    }
    else if (content_type != NULL && (sscanf(content_type, "multipart/form-data; boundary=%s", multipart) == 1)) {
        mk_lua_multipart_to_table(L, multipart, data);
    }
  
    mk_api->mem_free(content_type);
    mk_api->mem_free(data);
    return 2;
}

void mk_lua_inject_response(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "headers");
    lua_setfield(L, -2, "response"); /* initialising response table */
}

void mk_lua_set_response(lua_State *L)
{
    struct session_request *sr = mk_lua_get_session_request(L);
    lua_getglobal(L, "mk");
    lua_getfield(L, -1, "response");
    lua_getfield(L, -1, "status");
    if(!lua_isnil(L, -1)) {
        int isnum;
        int status = lua_tounsignedx(L, -1, &isnum);
        if(isnum) mk_api->header_set_http_status(sr, status);
    }
    else {
        mk_api->header_set_http_status(sr, 200); /* default status in 200 */
    }
    lua_pop(L,1);
    lua_getfield(L, -1, "headers");
        
    lua_pushnil(L);  /* first key */
    char* header;
    long unsigned int len;
    while (lua_next(L, -2) != 0) {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        header = NULL;
        mk_api->str_build(&header,
                          &len,
                          "%s: %s",
                          luaL_checkstring(L, -2),
                          luaL_checkstring(L, -1));
        mk_api->header_add(sr, header, len);
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(L, 1);
    }
}

void mk_lua_inject_request(lua_State *L)
{
    struct client_session *cs = mk_lua_get_client_session(L);
    struct session_request *sr = mk_lua_get_session_request(L);
    mk_ptr_t key, value;
    struct sockaddr_in addr;
    socklen_t addr_len;
    char buffer[128];
    char *tmpuri = NULL;
    int len;
    unsigned int i, j;
    char *hinit, *hend;
    size_t hlen;

    lua_newtable(L);              /* request table */

    lua_pushstring(L, sr->host_conf->host_signature);
    lua_setfield(L, -2, "server_software");

    __lua_pushmkptr(L, sr->host_conf->documentroot);
    lua_setfield(L, -2, "document_root");

    __lua_pushmkptr(L, sr->protocol_p);
    lua_setfield(L, -2, "server_protocol");

    lua_pushstring(L, sr->host_alias->name);
    lua_setfield(L, -2, "server_name");

    if (!getsockname(cs->socket, (struct sockaddr *)&addr, &addr_len)) {
        if (!inet_ntop(AF_INET, &addr.sin_addr, buffer, 128)) {
            log_warn("Failed to get bound address.");
            buffer[0] = '\0';
        }
        lua_pushstring(L, buffer);
        lua_setfield(L, -2, "server_addr");
        
        lua_pushnumber(L, ntohs(addr.sin_port));
        lua_setfield(L, -2, "server_port");
		
    } else {
        log_warn("%s", clean_errno());
        errno = 0;
    }       

    __lua_pushmkptr(L, sr->real_path);
    lua_setfield(L, -2, "script_filename");

    __lua_pushmkptr(L, sr->uri_processed);
    lua_setfield(L, -2, "script_name");

    __lua_pushmkptr(L, sr->method_p);
    lua_setfield(L, -2, "request_method");

    addr_len = sizeof(addr);
    if (!getpeername(cs->socket, (struct sockaddr *)&addr, &addr_len)) {
        inet_ntop(AF_INET, &addr.sin_addr, buffer, 128);

        lua_pushstring(L, buffer);
        lua_setfield(L, -2, "remote_addr");
        
        lua_pushnumber(L, ntohs(addr.sin_port));
        lua_setfield(L, -2, "remote_port");

    } else {
        log_warn("%s", clean_errno());
        errno = 0;
    }


    if (sr->query_string.len > 0) {
        len = sr->uri.len + sr->query_string.len + 2;
        tmpuri = mk_api->mem_alloc(len);
        check_mem(tmpuri);
        snprintf(tmpuri, len, "%.*s?%.*s",
                 (int)sr->uri.len, sr->uri.data,
                 (int)sr->query_string.len, sr->query_string.data);
        lua_pushstring(L, tmpuri);
    } else {
        __lua_pushmkptr(L, sr->uri);
    }
    lua_setfield(L, -2, "request_uri");

    __lua_pushmkptr(L, sr->query_string);
    lua_setfield(L, -2, "query_string");
    
    __lua_pushmkptr(L, sr->data);
    lua_setfield(L, -2, "data");

    lua_newtable(L);
    // headers table
    
    strcpy(buffer, "HTTP_");

    for (i = 0; i < (unsigned int)sr->headers_toc.length; i++) {
        hinit = sr->headers_toc.rows[i].init;
        hend = sr->headers_toc.rows[i].end;
        hlen = hend - hinit;

        for (j = 0; j < hlen; j++) {
            if (hinit[j] == ':') {
                break;
            }
            else if (hinit[j] != '-') {
                buffer[5 + j] = toupper(hinit[j]);
            }
            else {
                buffer[5 + j] = '_';
            }
        }

        key = (mk_ptr_t){.len = 5 + j, .data = buffer};
        value = (mk_ptr_t){.len = hlen - j - 2, .data = hinit + j + 2};

        __lua_pushmkptr(L, key);
        __lua_pushmkptr(L, value);
        lua_settable(L,-3);
    }

    lua_setfield(L, -2, "headers")
    
    lua_pushcfunction(L, mk_lua_query_to_table);
    lua_setfield(L, -2, "parseargs");

    lua_pushcfunction(L, mk_lua_data_to_table);
    lua_setfield(L, -2, "parsedata");

 error:
    if (tmpuri) mk_api->mem_free(tmpuri);
    lua_setfield(L, -2, "request"); /* setting request field */
}
