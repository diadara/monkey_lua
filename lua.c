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
#include <string.h>
#include <regex.h>

#include "MKPlugin.h"

#include <lua.h>
#include <lauxlib.h>


MONKEY_PLUGIN("lua",              /* shortname */
              "Lua scripting support plugin",    /* name */
              VERSION,             /* version */
              MK_PLUGIN_STAGE_30); /* hooks */


struct lua_match_t {
    regex_t match;
    
    struct mk_list _head;
};

struct lua_vhost_t {
    struct host* host;
    struct mk_list matches;
};



struct lua_vhost_t *lua_vhosts;
struct mk_list lua_global_matches;



static void str_to_regex(char *str, regex_t *reg) /* taken from cgi plugin */
{
    char *p = str;
    while (*p) {
        if (*p == ' ') *p = '|';
        p++;
    }

    int ret = regcomp(reg, str, REG_EXTENDED|REG_ICASE|REG_NOSUB);
    if (ret) {
        char tmp[80];
        regerror(ret, reg, tmp, 80);
        mk_err("CGI: Failed to compile regex: %s", tmp);
    }
}


static int mk_lua_link_matches(struct mk_config_section *section, struct mk_list *list)
{
    int n = 0;
    struct mk_list *head;
    struct mk_config_entry *entry;
    struct lua_match_t *match_line = NULL;

    mk_list_foreach(head, &section->entries) {
        entry = mk_list_entry(head, struct mk_config_entry, _head);
        if(strncasecmp(entry->key,"Match", strlen(entry->key)) ==0) {
            if(!entry->val) {
                mk_err("LUA: Invalid configuration value");
                exit(EXIT_FAILURE);
            }

            match_line = mk_api->mem_alloc_z(sizeof(struct lua_match_t));
            mk_list_add(&match_line->_head, list);
            str_to_regex(entry->val,&match_line->match);

            n++;
        }
    }
    return n;
}

static void mk_lua_config(const char * path)
{
    char *default_file = NULL;
    unsigned long len;
    struct mk_config *conf;
    struct mk_config_section * section;

    mk_api->str_build(&default_file, &len, "%slua.conf", path);
    conf = mk_api->config_create(default_file);
    section = mk_api->config_section_get(conf, "LUA");

    if(section) {
        mk_lua_link_matches(section, &lua_global_matches);
    }
        

    mk_api->mem_free(default_file);
    mk_api->config_free(conf);

    //Plugin global configuration should be done by now.Check for virtual
    //hosts next.
   

}

int _mkp_init(struct plugin_api **api, char *confdir)
{
    mk_api = *api;

    mk_list_init(&lua_global_matches);
    mk_lua_config(confdir);

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
    (void) plugin;
    (void)  cs;
    (void) sr;
    PLUGIN_TRACE("[FD %i] Handler received request in lua plugin");

    return MK_PLUGIN_RET_NOT_ME;
}
