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
#include <fcntl.h>
#include "MKPlugin.h"
#include "mk_lua.h"


MONKEY_PLUGIN("lua",              /* shortname */
              "Lua scripting support plugin",    /* name */
              VERSION,             /* version */
              MK_PLUGIN_STAGE_30); /* hooks */



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
        if (strncasecmp(entry->key,"Match", strlen(entry->key)) ==0) {
            if (!entry->val) {
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
    char *tmp = NULL;
    
    mk_api->str_build(&default_file, &len, "%slua.conf", path);
    conf = mk_api->config_create(default_file);
    section = mk_api->config_section_get(conf, "LUA");

    if (section) {
        mk_lua_link_matches(section, &lua_global_matches);
        tmp = mk_api->config_section_getval(section, "Debug", MK_CONFIG_VAL_BOOL);
        if (tmp)
            global_debug = MK_TRUE;
        else
            global_debug = MK_FALSE;
    }

    

    mk_api->mem_free(default_file);
    mk_api->config_free(conf);

    //Plugin global configuration should be done by now.Check for virtual
    //hosts next.

    struct mk_list *hosts = &mk_api->config->hosts;
    struct mk_list *head_host;
    struct host *entry_host;
    unsigned short vhosts = 0;

    mk_list_foreach(head_host, hosts) {
        entry_host = mk_list_entry(head_host, struct host, _head);
        section = mk_api->config_section_get(entry_host->config, "LUA");
        if (section) vhosts++;
    }

    if (vhosts < 1) return;

    // NULL-terminated linear cache
    lua_vhosts = mk_api->mem_alloc_z((vhosts + 1) * sizeof(struct lua_vhost_t));

    vhosts = 0;
    mk_list_foreach(head_host, hosts) {
        entry_host = mk_list_entry(head_host, struct host, _head);
        section = mk_api->config_section_get(entry_host->config, "LUA");

        if (!section) {
            continue;
        }

        /* Set the host for this entry */
        lua_vhosts[vhosts].host = entry_host;
        mk_list_init(&lua_vhosts[vhosts].matches);

        /*
         * For each section found on every virtual host, lookup all 'Match'
         * keys and populate the list of scripting rules
         */
        mk_lua_link_matches(section, &lua_vhosts[vhosts].matches);

        /* add debug to this vhost */
        /* if there is no debug defined, config will return off */


        tmp =  mk_api->config_section_getval(section, "Debug", MK_CONFIG_VAL_BOOL);
        if (tmp)
            lua_vhosts[vhosts].debug = MK_TRUE;
        else
            lua_vhosts[vhosts].debug = MK_FALSE;
        vhosts++;
    }
    
}



int _mkp_init(struct plugin_api **api, char *confdir)
{
    mk_api = *api;

    mk_list_init(&lua_global_matches);
    mk_lua_config(confdir);
    pthread_key_create(&lua_request_list, NULL);
    getrlimit(RLIMIT_NOFILE, &lim);
    requests_by_socket = mk_api->mem_alloc_z(sizeof(struct lua_request *) * lim.rlim_cur);

    return 0;
}

void _mkp_exit()
{
    mk_api->mem_free(requests_by_socket);
}

void _mkp_core_thctx(void)
{
    struct mk_list *list = mk_api->mem_alloc_z(sizeof(struct mk_list));

    mk_list_init(list);
    pthread_setspecific(lua_request_list, (void *) list);
}

void mk_lua_send(struct client_session *cs,
                 struct session_request *sr,
                 const char * buffer) {
    UNUSED_VARIABLE(sr);
    int ret_len = strlen(buffer);
    fcntl(cs->socket, F_SETFL, fcntl(cs->socket, F_GETFL, 0) & ~O_NONBLOCK);
    mk_api->socket_set_nonblocking(cs->socket);
    mk_api->socket_send(cs->socket, buffer, ret_len);

}

/* Object handler */
int _mkp_stage_30(struct plugin *plugin,
                  struct client_session *cs,
                  struct session_request *sr)
{
    UNUSED_VARIABLE(cs);
    UNUSED_VARIABLE(plugin);
    PLUGIN_TRACE("[FD %i] Handler received request in lua plugin");
    unsigned int i;
    char url[PATHLEN];
    struct lua_match_t * match_rule;
    struct mk_list *head_matches;

    if (sr->uri.len +1 > PATHLEN)
        return MK_PLUGIN_RET_NOT_ME;

    memcpy(url, sr->uri.data, sr->uri.len);
    url[sr->uri.len] = '\0';

    const char *const file  = sr->real_path.data;

    /* can't interpret script if not a file */
    if (!sr->file_info.is_file) {
        return MK_PLUGIN_RET_NOT_ME;
    }

    /* check if request matches global lua match patterns */
    mk_list_foreach(head_matches, &lua_global_matches) {
        match_rule = mk_list_entry(head_matches, struct lua_match_t, _head);
        if (regexec(&match_rule->match, url, 0, NULL, 0) == 0) {
            goto run_lua;
        }
    }
    /* Check for rules in virltual host */
    if (!lua_vhosts) {
        return MK_PLUGIN_RET_NOT_ME;
    }

    for (i = 0; lua_vhosts[i].host; i++) {
        if(sr->host_conf == lua_vhosts[i].host) {
            break;
        }
    }
    /* if No vhost with lua match the request */
    if (!lua_vhosts[i].host) {
        return MK_PLUGIN_RET_NOT_ME;
    }

    /* vhost found, check regex */
    mk_list_foreach(head_matches, &lua_vhosts[i].matches) {
        match_rule = mk_list_entry(head_matches, struct lua_match_t, _head);
        if (regexec(&match_rule->match, url, 0, NULL, 0) == 0) {
            goto run_lua;
        }
    }

    /* We reach here only if no match was found */
    return MK_PLUGIN_RET_NOT_ME;

 run_lua:

    int status = do_lua(file, sr, cs, plugin);

    /* These are just for the other plugins, such as logger; bogus data */
    mk_api->header_set_http_status(sr, status);

    if (status != 200)
        return MK_PLUGIN_RET_CLOSE_CONX;

    sr->headers.cgi = SH_CGI;

    return MK_PLUGIN_RET_CONTINUE;

}


void * Lua_excecution(void *r)
{
    int status_load, status_run;
    status_load = luaL_loadfile(L, file);
    int status_load, status_run;
    status_load = luaL_loadfile(L, file);
    if (status_load != LUA_OK) {
        mk_api->header_set_http_status(sr, 500);
        mk_api->header_send(cs->socket, cs, sr);
        if(global_debug || lua_vhosts[i].debug)
            mk_lua_send(cs, sr, lua_tostring(L, -1));
        goto cleanup;
    }
    else {
        status_run = lua_pcall(L, 0, 0, lua_gettop(L) - 1);
    }

    if (status_run == LUA_OK) {
        mk_lua_post_execute(L);
    }
    else {
        mk_api->header_set_http_status(sr, 500);
    }
    
    if (status_run == LUA_OK || (global_debug || lua_vhosts[i].debug))
        {
            char *header = NULL;
            unsigned long int len;
            mk_api->str_build(&header,
                              &len,
                              "Content-length : %d",
                             (int)strlen(mk_lua_return));
            mk_api->header_add(sr, header, len);
            mk_api->header_send(cs->socket, cs, sr);
            free(header);
            mk_lua_send(cs, sr, mk_lua_return);
        }



}

int do_lua(const char *const file,
           struct session_request *const sr,
           struct client_session *const cs,
           struct plugin* const plugin)
{


    lua_State *L = mk_lua_init_env(cs, sr); 
    lua_request *r = lua_req_create(L, cs->socket, sr, cs);
    mk_api->worker_spawn(lua_excecution, r);

    /* We have nothing to write yet */
    mk_api->event_socket_change_mode(socket, MK_EPOLL_SLEEP, MK_EPOLL_LEVEL_TRIGGERED);

    return 200;
}

struct lua_request *lua_req_create(lua_State *L, int socket, struct session_request *sr,
					struct client_session *cs)
{
    struct lua_request *newlua = mk_api->mem_alloc_z(sizeof(struct lua_request));
    if (!newlua) return NULL;

    newlua->L = L;
    newlua->socket = socket;
    newlua->sr = sr;
    newlua->cs = cs;

    return newlua;
}

void lua_req_add(struct lua_request *r)
{
    struct mk_list *list = pthread_getspecific(lua_request_list);

    mk_bug(!list);
    mk_list_add(&r->_head, list);
}

int lua_req_del(struct lua_request *r)
{
    if (!r) return 1;

    mk_list_del(&r->_head);
    mk_api->mem_free(r);

    return 0;
}


static int hangup(const int socket)
{

  if ((r = cgi_req_get(socket))) {

        /* If this was closed by us, do nothing */
        if (!requests_by_socket[r->socket])
            return MK_PLUGIN_RET_EVENT_OWNED;


        /* XXX Fixme: this needs to be atomic */
        requests_by_socket[r->socket] = NULL;

        lua_req_del(r);

        return MK_PLUGIN_RET_EVENT_OWNED;
    }

    return MK_PLUGIN_RET_EVENT_CONTINUE;
}

int _mkp_event_write(int socket)
{
    struct cgi_request *r = cgi_req_get(socket);
    if (!r) return MK_PLUGIN_RET_EVENT_NEXT;

    if (r->in_len > 0) {

        mk_api->socket_cork_flag(socket, TCP_CORK_ON);

        const char * const buf = r->in_buf, *outptr = r->in_buf;

        mk_api->header_send(socket, r->cs, r->sr);
        r->status_done = 1;
        
        mk_api->socket_cork_flag(socket, TCP_CORK_OFF);
    }

    return MK_PLUGIN_RET_EVENT_OWNED;
}



int _mkp_event_error(int socket)
{
    hangup(socket);
}

int _mkp_event_close(int socket)
{
    hangup(socket);
}

