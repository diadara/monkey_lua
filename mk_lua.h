#ifndef _MK_LUA
#define _MK_LUA

#include <string.h>
#include <regex.h>

#include <lua.h>
#include <lauxlib.h>
#include "MKPlugin.h"



#define UNUSED_VARIABLE(var) (void)(var)

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
};



struct lua_vhost_t *lua_vhosts;
struct mk_list lua_global_matches;

extern char *mk_lua_return;

lua_State * mk_lua_init_env(struct client_session *cs,
                            struct session_request *sr);
void mk_lua_post_execute(lua_State *L,
                         struct client_session *cs,
                         struct session_request *sr);
#endif
