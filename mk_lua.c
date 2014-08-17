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
#include <sys/resource.h>
#include "monkey/mk_api.h"
#include "mk_lua.h"


MONKEY_PLUGIN("lua",              /* shortname */
              "Lua scripting support plugin",    /* name */
              VERSION,             /* version */
              MK_PLUGIN_STAGE_30 | MK_PLUGIN_CORE_THCTX); /* hooks */


pthread_key_t mk_lua_worker_ctx_key;

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

void mk_lua_printc(struct lua_request *r, char *buf)
{
    int len;
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

}

void mk_lua_resume_execution(struct lua_request *r)
{
    lua_State *vm = mk_lua_get_lua_vm();
    
    int status_run = lua_resume(r->co, vm, 0);
    
    if (status_run == LUA_OK) {
        /* the script has finished execution.*/
        r->lua_status = MK_LUA_OK;
        
        mk_lua_post_execute(r->co);

        
        /* We are ready to wrte the response */
        mk_api->event_socket_change_mode(r->socket, MK_EPOLL_WRITE, MK_EPOLL_LEVEL_TRIGGERED);
    }
    else if (status_run == LUA_YIELD) {

        MK_TRACE("[FD %i] Script has yielded control to event loop", r->socket);
        
        /* TODO get the fd for the blocking call and register it
           with monkey even loop, create a maping for the request
           to the file discriptor. When the blocking calls state
           changes, resume the coroutine */
        
    }
    else {
        r->lua_status = MK_LUA_RUN_ERROR;
        MK_TRACE("[FD %i] lua script %s failed to execute successfully", r->socket, r->file);
        printf("Error:  %s\n", lua_tostring(r->co, -1));
        free(r->buf);
        r->buf = mk_api->str_dup(lua_tolstring(r->co, -1, &r->in_len));
    }
}


void  mk_lua_process_request(void *req)
{
    struct lua_request *r = (struct lua_request*) req;
    struct client_session *cs = r->cs;
    struct session_request *sr = r->sr;

    lua_State *co = r->co;
    lua_State *vm = mk_lua_get_lua_vm();
    
    const char* file = r->file;

    int status_load, status_run;

    /* TODO isolate the coroutine from vm and prevent it from
       manipulating the global vm */
    
    status_load = luaL_loadfile(co, file);

    if (status_load != LUA_OK) {
        r->lua_status = MK_LUA_LOAD_ERROR;
        
    }
    else {
        mk_lua_resume_execution(r);
    }
    

   
}


int do_lua(const char *const file,
           struct session_request *const sr,
           struct client_session *const cs,
           struct plugin* const plugin,
           int debug)
{

    struct mk_lua_worker_ctx * ctx = pthread_getspecific(mk_lua_worker_ctx_key);
    lua_State *L = ctx->L;
    lua_State *co = lua_newthread(L);

    
    struct lua_request *r = lua_req_create(co, file, cs->socket, sr, cs, debug);

    

    /* We have nothing to write yet */
    mk_api->event_socket_change_mode(cs->socket, MK_EPOLL_SLEEP,  MK_EPOLL_LEVEL_TRIGGERED);


    
    /* Wake up the socket once we have data to write */
    //    mk_api->event_socket_change_mode(cs->socket, MK_EPOLL_WAKEUP,  MK_EPOLL_LEVEL_TRIGGERED);
    // mk_api->event_socket_change_mode(cs->socket, MK_EPOLL_WAKEUP,  MK_EPOLL_LEVEL_TRIGGERED);


    /* deal with chunking the response later */

    /* if (r->sr->protocol >= MK_HTTP_PROTOCOL_11 && */
    /*     (r->sr->headers.status < MK_REDIR_MULTIPLE || */
    /*      r->sr->headers.status > MK_REDIR_USE_PROXY)) */
    /*     { */
    /*         r->sr->headers.transfer_encoding = MK_HEADER_TE_TYPE_CHUNKED; */
    /*         r->chunked = 1; */
    /*     } */

    
    /* add request to the processing list */
    lua_req_add(r);

    /* start processing the request */
    mk_lua_process_request(r);
    
    /* wakeup socket */
    mk_api->event_socket_change_mode(cs->socket, MK_EPOLL_WAKEUP,  MK_EPOLL_LEVEL_TRIGGERED);
    
    return 200;
}



int _mkp_init(struct plugin_api **api, char *confdir)
{
    mk_api = *api;
    struct rlimit lim;
    
    mk_list_init(&lua_global_matches);
    mk_lua_config(confdir);
    
    pthread_key_create(&mk_lua_worker_ctx_key, NULL);
    
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
    struct mk_lua_worker_ctx * worker_ctx = mk_api->mem_alloc_z(sizeof(struct mk_lua_worker_ctx));

    mk_lua_init_worker_env(worker_ctx);
    pthread_setspecific(mk_lua_worker_ctx_key, (void *) worker_ctx);
}


void mk_lua_send(struct client_session *cs,
                 struct session_request *sr,
                 const char * buffer) {
    UNUSED_VARIABLE(sr);
    int ret_len = strlen(buffer);
    //    mk_api->socket_set_nonblocking(cs->socket);
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
    int debug = MK_FALSE;
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
            debug = lua_vhosts[i].debug;
            goto run_lua;
        }
    }

    /* We reach here only if no match was found */
    return MK_PLUGIN_RET_NOT_ME;

 run_lua:

    debug = (global_debug || debug) ? MK_TRUE : MK_FALSE ;
  
    int status = do_lua(file, sr, cs, plugin, debug); 

    /* /\* These are just for the other plugins, such as logger; bogus data *\/ */
    /* mk_api->header_set_http_status(sr, status); */

    /* if (status != 200) */
    /*     return MK_PLUGIN_RET_CLOSE_CONX; */

    /* sr->headers.cgi = SH_CGI; */

        return MK_PLUGIN_RET_CONTINUE;

        //    return MK_PLUGIN_RET_CLOSE_CONX;
    
}


struct lua_request *lua_req_create(lua_State *co,
                                   const char *file,
                                   int socket,
                                   struct session_request *sr,
                                   struct client_session *cs,
                                   int debug)
{
    struct lua_request *newlua = mk_api->mem_alloc_z(sizeof(struct lua_request));
    if (!newlua) return NULL;

    newlua->co = co;
    newlua->file = file;
    newlua->socket = socket;
    newlua->sr = sr;
    newlua->cs = cs;
    newlua->buf = NULL;
    newlua->in_len = 0;
    newlua->debug = debug;
    /* acesss the request structure from the lua_state instead of
       creating an array to keep track of things */
    lua_pushlightuserdata(co, (void *) newlua);
    lua_setglobal(co, "__mk_lua_req");

    return newlua;
}

void lua_req_add(struct lua_request *r)
{
    requests_by_socket[r->socket] = r;
    MK_TRACE("[FD %i] adding request to lua request array", r->socket);
}


int lua_req_del(struct lua_request *r, int socket)
{
    mk_api->mem_free(r);
    requests_by_socket[socket] = NULL;
    return 0;
}



int _mkp_event_write(int socket)
{
    struct lua_request *r = lua_req_get(socket);
    if (!r) {
        return MK_PLUGIN_RET_EVENT_NEXT;
    }


    printf("about to write the response for the request");
    struct client_session *cs = r->cs;
    struct session_request *sr = r->sr;
    
    /* mk_api->header_set_http_status(sr, 200); */
    /* sr->headers.content_length = strlen("Hello World"); */
    /* mk_api->header_send(cs->socket, cs, sr); */
    /* mk_lua_send(cs, sr, "Hello World"); */


  
    if(r->lua_status != MK_LUA_OK) {
        mk_api->header_set_http_status(sr, 500);
        MK_TRACE("[FD %i] script failed to execute", r->socket);
    }
    
    if (((r->lua_status == MK_LUA_RUN_ERROR) && r->debug) || r->lua_status == MK_LUA_OK) {

        sr->headers.content_length = (int)r->in_len;
        mk_api->header_send(cs->socket, cs, sr);
        mk_lua_send(cs, sr, r->buf);

        printf("response: %s", r->buf);
    }
    else
        {
            printf("don't know");
            sr->headers.content_length = 0;
            mk_api->header_send(cs->socket, cs, sr);
        }


    lua_req_del(r, socket);

    mk_api->event_socket_change_mode(socket, MK_EPOLL_SLEEP, MK_EPOLL_LEVEL_TRIGGERED);
    mk_api->event_socket_change_mode(socket, MK_EPOLL_READ, MK_EPOLL_LEVEL_TRIGGERED);


    return MK_PLUGIN_RET_EVENT_OWNED;
}


