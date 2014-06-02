#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "mk_lua.h"
#include <stdio.h>

char *mk_lua_return = NULL;

extern struct plugin_api *mk_api;

static int mk_lua_print(lua_State *L)
{
    char *buf = luaL_checkstring(L, 1);
    long unsigned int len;
    //    mk_api->str_build(&mk_lua_return, &len, "%s%s",mk_lua_return,buf);
    printf("%s\n",buf);
    return 0;
}

lua_State * mk_lua_init_env()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    lua_pushstring(L, "worker_capacity");
    lua_pushinteger(L, mk_api->config->worker_capacity);
    lua_settable(L, -3);
    lua_setglobal(L,"monkey");


    static const struct luaL_Reg mk_lua_lib []  ={
        {"print", mk_lua_print},
        {NULL, NULL}
    };

    luaL_setfuncs(L, mk_lua_lib,0);
    return L;
}
