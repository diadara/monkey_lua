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
    if (mk_lua_return) {
      char *temp=NULL;
      mk_api->str_build(&temp, &len, "%s\n%s",mk_lua_return,buf);
      mk_api->mem_free(mk_lua_return);
      mk_lua_return = temp;
    }
      else
      mk_lua_return = mk_api->str_dup(buf);

    return 0;
}

lua_State * mk_lua_init_env()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    
    /* Register these functions */
    static const struct luaL_Reg mk_lua_lib []  ={
        {"print", mk_lua_print},
        {NULL, NULL}
    };
    
    luaL_newlib(L, mk_lua_lib);
    lua_pushstring(L, "worker_capacity");
    lua_pushinteger(L, mk_api->config->worker_capacity);
    lua_settable(L, -3);
    lua_setglobal(L,"monkey");

    return L;
}
