/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef _MK_LUA
#define _MK_LUA

#include <string.h>
#include <regex.h>

#include <lua.h>
#include <lauxlib.h>
#include "monkey/mk_api.h"

#include "mk_lua_config.h"
#include "mk_lua_util.h"
#include "mk_lua_request.h"
#include "mk_lua_cookie.h"

#define UNUSED_VARIABLE(var) (void)(var)

#define __lua_pushmkptr(L, mk_ptr) lua_pushlstring(L, mk_ptr.data, mk_ptr.len)

#define MK_LUA_OK 0
#define MK_LUA_LOAD_ERROR 1
#define MK_LUA_RUN_ERROR 2

#define MK_LUA_DEBUG 1
#define MK_LUA_NO_DEBUG 0

enum {
    PATHLEN = 1024,
    SHORTLEN = 64
};

struct lua_match_t {
    regex_t match;
    
    struct mk_list _head;
};

struct lua_vhost_t {
    struct host* host;
    struct mk_list matches;
    int debug;
};

struct mk_lua_worker_ctx {

    lua_State *L;
    
};
    
struct lua_request {

    char in_buf[PATHLEN];
    struct mk_list _head;

    struct session_request *sr;
    struct client_session *cs;

    unsigned int in_len;
    char *buf;
    
    int socket;
    int fd;
    lua_State *co;
    const char *file;
    int lua_status;
    int debug;
    unsigned char status_done;
    unsigned char all_headers_done;
    unsigned char chunked;

};



int global_debug;

struct lua_vhost_t *lua_vhosts;
struct mk_list lua_global_matches;



lua_State * mk_lua_init_request_env(lua_State* L,
                                    struct client_session *cs,
                                    struct session_request *sr);
int mk_lua_init_worker_env(struct mk_lua_worker_ctx * ctx);
void mk_lua_post_execute(lua_State *L);



extern pthread_key_t mk_lua_worker_ctx_key;



struct lua_request **requests_by_socket;

struct lua_request *lua_req_create(lua_State *L,
                                   const char *file,
                                   int socket,
                                   struct session_request *sr,
                                   struct client_session *cs,
                                   int debug);


void lua_req_add(struct lua_request *r);
int lua_req_del(struct lua_request *r, int socket);

// Get the LUA request by the client socket
static inline struct lua_request *lua_req_get(int socket)
{
    MK_TRACE("[FD %i] looking up request", socket);
    struct lua_request *r = requests_by_socket[socket];
    return r;
}


static inline struct lua_State* mk_lua_get_lua_vm()
{
    struct mk_lua_worker_ctx * ctx = pthread_getspecific(mk_lua_worker_ctx_key);
    lua_State *vm =  ctx->L;
    return vm;
}

void mk_lua_printc(struct lua_request *r, char *buf);

#endif




