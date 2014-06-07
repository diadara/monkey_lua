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

  lua_pushinteger(L, config->max_load);
  lua_setfield(L, -2, "max_load");

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
 
lua_State * mk_lua_init_env(struct session_request *sr)
{
  (void)sr;
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  
  /* Register these functions */
  static const struct luaL_Reg mk_lua_lib []  ={
    {"print", mk_lua_print},
    {NULL, NULL}
  };
    
  luaL_newlib(L, mk_lua_lib); /* registers all the functions */
  mk_lua_init_env_config(L);  
  lua_setglobal(L,"monkey");

  return L;
}
