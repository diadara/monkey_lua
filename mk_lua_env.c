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


#define FORM_URLENCODED "application/x-www-form-urlencoded"

char *mk_lua_return = NULL;

static int mk_lua_print(lua_State *L)
{
    const char *buf = luaL_checkstring(L, 1);
    long unsigned int len;

    struct lua_request *r =  mk_lua_get_lua_request(L);
    char* mk_lua_return = r->buf;
    if (mk_lua_return) {
        char *temp = NULL;
        mk_api->str_build(&temp, &len, "%s\n%s", mk_lua_return, buf);
        mk_api->mem_free(mk_lua_return);
        mk_lua_return = temp;
    }
    else
        mk_lua_return = mk_api->str_dup(buf);

    r->buf = mk_lua_return;
    r->in_len = strlen(mk_lua_return);
    return 0;
}



static int mk_lua_traceback(lua_State *L)
{
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);      /* skip this function and the traceback */
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
    mk_lua_inject_config(L);
    mk_lua_inject_request(L);
    mk_lua_inject_response(L);
    mk_lua_inject_cookies(L);
    lua_setglobal(L,"mk");

    return L;
}


void mk_lua_post_execute(lua_State *L)
{
    mk_lua_set_response(L);
}

