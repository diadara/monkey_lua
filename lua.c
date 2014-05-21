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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "MKPlugin.h"

#include <lua.h>
#include <lauxlib.h>


MONKEY_PLUGIN("lua",              /* shortname */
              "Lua scripting support plugin",    /* name */
              VERSION,             /* version */
              MK_PLUGIN_STAGE_30); /* hooks */


int _mkp_init(struct plugin_api **api, char *confdir)
{
    (void) confdir;
    mk_api = *api;

    return 0;
}

void _mkp_exit()
{
}



/* Object handler */
int _mkp_stage_30(struct plugin *plugin,
                  struct client_session *cs,
                  struct session_request *sr)
{
    int val;
    short int is_restricted = MK_FALSE;
    mk_ptr_t res;
    (void) plugin;
    (void)  cs;
    (void) sr;
    PLUGIN_TRACE("[FD %i] Handler received request");
    return MK_PLUGIN_RET_NOT_ME;
}
