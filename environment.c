/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h> /* getsockname, getpeername */
#include <arpa/inet.h> /* inet_ntop */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "mk_lua.h"
#include "dbg.h"

#define __lua_pushmkptr(L, mk_ptr) lua_pushlstring(L, mk_ptr.data, mk_ptr.len)

#define FORM_URLENCODED "application/x-www-form-urlencoded"

char *mk_lua_return = NULL;

extern struct plugin_api *mk_api;

static int mk_lua_print(lua_State *L)
{
    char *buf = luaL_checkstring(L, 1);
    long unsigned int len;
    if (mk_lua_return) {
        char *temp = NULL;
        mk_api->str_build(&temp, &len, "%s\n%s", mk_lua_return, buf);
        mk_api->mem_free(mk_lua_return);
        mk_lua_return = temp;
    }
    else
        mk_lua_return = mk_api->str_dup(buf);

    return 0;
}

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
    struct session_request *sr = (struct session_request *) lua_touserdata(L, lua_upvalueindex(1));
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

void mk_lua_init_env_response(lua_State *L, struct session_request *sr)
{
    UNUSED_VARIABLE(sr);
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "headers");
    lua_setfield(L, -2, "response"); /* initialising response table */
}

void mk_lua_set_response(lua_State *L, struct session_request *sr)
{
    lua_getglobal(L, "mk");
    lua_getfield(L, -1, "response");
    lua_getfield(L, -1, "status");
    if(!lua_isnil(L, -1)) {
        int isnum;
        int status = lua_tounsignedx(L, -1, &isnum);
        if(isnum) mk_api->header_set_http_status(sr, status);
    }
    else {
        mk_api->header_set_http_status(sr, 200);
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

void mk_lua_init_env_request(lua_State *L,
                             struct client_session *cs,
                             struct session_request *sr)
{
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

    lua_pushcfunction(L, mk_lua_query_to_table);
    lua_setfield(L, -2, "parseargs");

    lua_pushlightuserdata(L, (void *) sr);
    lua_pushcclosure(L, mk_lua_data_to_table, 1);
    lua_setfield(L, -2, "parsedata");

 error:
    if (tmpuri) mk_api->mem_free(tmpuri);
    lua_setfield(L, -2, "request"); /* setting request field */
}

void mk_lua_init_env_config(lua_State *L)
{
    struct server_config *config = mk_api->config;
    lua_newtable(L);              /* configtable */


    lua_newtable(L);              /* kernel_features */
    lua_pushboolean(L, config->kernel_features & MK_KERNEL_TCP_FASTOPEN);
    lua_setfield(L, -2, "TCP_FASTOPEN");
    lua_pushboolean(L, config->kernel_features & MK_KERNEL_SO_REUSEPORT);
    lua_setfield(L, -2,  "SO_REUSEPORT");
    lua_pushboolean(L, config->kernel_features & MK_KERNEL_TCP_AUTOCORKING);
    lua_setfield(L, -2, "TCP_AUTOCORKING");
    lua_setfield(L, -2, "kernel_features");
  
    lua_pushstring(L, "worker_capacity");
    lua_pushinteger(L, config->worker_capacity);
    lua_settable(L, -3);

    lua_pushinteger(L, config->max_load);
    lua_setfield(L, -2, "max_load");

    lua_pushinteger(L, config->workers);
    lua_setfield(L, -2, "workers");

    lua_pushboolean(L, config->manual_tcp_cork);
    lua_setfield(L, -2, "manual_tcp_cork");

    lua_pushboolean(L, config->fdt);
    lua_setfield(L, -2, "fdt");

    lua_pushboolean(L, config->is_daemon);
    lua_setfield(L, -2, "is_daemon");

    lua_pushboolean(L, config->is_seteuid);
    lua_setfield(L, -2, "is_seteuid");

    if (config->scheduler_mode == MK_SCHEDULER_FAIR_BALANCING)
        lua_pushstring(L, "MK_SCHEDULER_FAIR_BALANCING");
    else if (config->scheduler_mode == MK_SCHEDULER_REUSEPORT)
        lua_pushstring(L, "MK_SCHEDULER_REUSEPORT");
    lua_setfield(L, -2, "scheduler_mode");

    lua_pushstring(L,config->serverconf);
    lua_setfield(L, -2, "serverconf");

    lua_pushstring(L, config->listen_addr);
    lua_setfield(L, -2, "listen_addr");

    lua_pushstring(L, config->listen_addr);
    lua_setfield(L, -2, "listen_addr");

    lua_pushlstring(L, config->server_addr.data, config->server_addr.len);
    //  lua_pushstring(L, mk_api->pointer_to_buf(config->server_addr));
    lua_setfield(L, -2, "server_addr");

    lua_pushlstring(L, config->server_software.data, config->server_software.len);
    lua_setfield(L, -2, "server_software");

    lua_pushstring(L, config->user);
    lua_setfield(L, -2, "user");

    lua_pushstring(L, config->user_dir);
    lua_setfield(L, -2, "user_dir");
 
    lua_pushstring(L, config->pid_file_path);
    lua_setfield(L, -2, "pid_file_path");

    lua_pushstring(L, config->server_conf_file);
    lua_setfield(L, -2, "server_conf_file");

    lua_pushstring(L, config->mimes_conf_file);
    lua_setfield(L, -2, "mimes_conf_file");

    lua_pushstring(L, config->plugin_load_conf_file);
    lua_setfield(L, -2, "plugin_load_conf_file");

    lua_pushstring(L, config->sites_conf_dir);
    lua_setfield(L, -2, "sites_conf_dir");

    lua_pushstring(L, config->plugins_conf_dir);
    lua_setfield(L, -2, "plugins_conf_dir");

    /* TODO rest of the server config, list of loaded plugins, list of
       virtual hosts */
  
    lua_pushstring(L, config->transport);
    lua_setfield(L, -2, "transport");

    lua_pushstring(L, config->transport_layer);
    lua_setfield(L, -2, "transport_layer");

    lua_setfield(L, -2, "config");

}

static int mk_lua_traceback(lua_State *L)
{
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    lua_getglobal(L,"mk");
    lua_getfield(L, -1, "print");
    lua_pushvalue(L, -3);
    lua_call(L, 1, 0);
    //  printf("\n%s\n", lua_tostring(L, -1));
    return 1;
}

lua_State * mk_lua_init_env(struct client_session *cs,
                            struct session_request *sr)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_pushcfunction(L, mk_lua_traceback);

    /* pushing cs and sr for other functions to get access */
    lua_pushlightuserdata(L, (void *) sr);
    lua_setglobal(L, "__mk_lua_sr");

    lua_pushlightuserdata(L, (void *) cs);
    lua_setglobal(L, "__mk_lua_cs");

    /* Register these functions */
    static const struct luaL_Reg mk_lua_lib []  ={
        {"print", mk_lua_print},
        {NULL, NULL}
    };
    
    luaL_newlib(L, mk_lua_lib); /* registers all the functions */
    mk_lua_init_env_config(L);
    mk_lua_init_env_request(L, cs, sr);
    mk_lua_init_env_response(L, sr);
    lua_setglobal(L,"mk");

    return L;
}


void mk_lua_post_execute(lua_State *L,
                         struct client_session *cs,
                         struct session_request *sr)
{
    UNUSED_VARIABLE(cs);
    mk_lua_set_response(L, sr);
}

