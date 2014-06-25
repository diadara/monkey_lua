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

void mk_lua_inject_config(lua_State *L)
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
