/*
 * NProxy - High performance HTTP Proxy Software/Library
 *
 * Copyright (C) 2008-2013, Massimo Cetra <massimo.cetra at gmail.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is NProxy
 *
 * The Initial Developer of the Original Code is
 * Massimo Cetra <massimo.cetra at gmail.com>
 *
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * libnproxy_lua.c
 *
 */

#include "libnproxy.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* Define the default as lua 5.1 (the most common) */
#ifndef LUA_VERSION_NUMBER
#define LUA_VERSION_NUMBER 51
#endif

static int lua_panic(lua_State * L) {
    nn_log(NN_LOG_ERROR, "Unprotected error in call to Lua API (%s)", lua_tostring(L, -1));
    return 0;
}

static int traceback(lua_State * L) {
#if LUA_VERSION_NUMBER >= 51
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
#else
    lua_pushstring(L, "debug");
    lua_gettable(L, LUA_GLOBALSINDEX);
#endif
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
#if LUA_VERSION_NUMBER >= 51
    lua_getfield(L, -1, "traceback");
#else
    lua_pushstring(L, "traceback");
    lua_gettable(L, -1);
#endif
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1);        /* pass error message */
#if LUA_VERSION_NUMBER >= 51
    lua_pushinteger(L, 2);      /* skip this function and traceback */
#else
    lua_pushnumber(L, 2);       /* skip this function and traceback */
#endif
    lua_call(L, 2, 1);          /* call debug.traceback */

    return 1;
}

static int docall(lua_State * L, int narg, int clear) {
    int status;
    int base = lua_gettop(L) - narg;    /* function index */

    lua_pushcfunction(L, traceback);    /* push traceback function */
    lua_insert(L, base);        /* put it under chunk and args */

    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);

    lua_remove(L, base);        /* remove traceback function */

#if LUA_VERSION_NUMBER >= 51
    if (status != 0) {
        lua_gc(L, LUA_GCCOLLECT, 0);    /* force a complete garbage collection in case of errors */
    }
#endif

    return status;
}

static lua_State *lua_init(void) {
    lua_State *L = lua_open();
    int error;

    if (L) {
        const char *buff = "os.exit = function() nproxy.logmsg(\"\", \"Surely you're joking! os.exit is a bad idea...\\n\") end";
#if LUA_VERSION_NUMBER >= 51
        lua_gc(L, LUA_GCSTOP, 0);
#endif
#if LUA_VERSION_NUMBER >= 51
        luaL_openlibs(L);
#else
        luaopen_base(L);        /* opens the basic library */
        luaopen_table(L);       /* opens the table library */
        luaopen_io(L);          /* opens the I/O library */
        luaopen_string(L);      /* opens the string lib. */
        luaopen_math(L);        /* opens the math lib. */
        luaopen_loadlib(L);
#endif
        luaopen_nproxy(L);
#if LUA_VERSION_NUMBER >= 51
        lua_gc(L, LUA_GCRESTART, 0);
#endif
        lua_atpanic(L, lua_panic);
        error = luaL_loadbuffer(L, buff, strlen(buff), "line") || docall(L, 0, 1);
        (void) error;
    }

    return L;
}

static void lua_uninit(lua_State * L) {
#if LUA_VERSION_NUMBER >= 51
    lua_gc(L, LUA_GCCOLLECT, 0);
#endif
    lua_close(L);
}

static int lua_loadscript(lua_State * L, char *fname) {
    if (luaL_loadfile(L, fname)
        || docall(L, 0, 1)
        ) {
        nn_log(NN_LOG_ERROR, "cannot run file: %s", lua_tostring(L, -1));
        return 0;
    }
    return 1;
}

/* *************************************************************************
   ************************************************************************* */

static void lua_connection_start(nproxy_connection_t * conn) {
    lua_State *L = NULL;

    if (conn->lua != NULL) {
        return;
    }

    if (zstr(conn->profile->luascript)) {
        return;
    }

    if (!(L = lua_init())) {
        return;
    }

    if (!lua_loadscript(L, conn->profile->luascript)) {
        return;
    }

    conn->lua = L;

    return;
}

void lua_connection_stop(nproxy_connection_t * conn) {
    if (!conn->lua) {
        return;
    }

    lua_uninit(conn->lua);
    conn->lua = NULL;
}

/* *************************************************************************
   ************************************************************************* */

int lua_call_connect(nproxy_connection_t * conn, char *ip) {
    lua_State *L = NULL;
    int ret = HTTP_INTERNAL_SERVER_ERROR;

    assert(conn);

    lua_connection_start(conn);

    if (!(L = conn->lua)) {
        return ret;
    }

    /* push functions and arguments */
    lua_getglobal(L, "on_connect"); /* function to be called */

    lua_push_conn(L, conn);
    lua_pushstring(L, ip);      /* push 2st argument */

    /* do the call (2 arguments, 1 result) */
    if (lua_pcall(L, 2, 1, 0) != 0) {
        nn_log(NN_LOG_ERROR, "error running function `%s': %s", "on_connect", lua_tostring(L, -1));
        /* pop returned value */
        lua_pop(L, 1);
    } else {
        /* retrieve result */
        if (!lua_isnumber(L, -1)) {
            nn_log(NN_LOG_ERROR, "function `on_connect' must return a number");
        } else {
            ret = lua_tonumber(L, -1);
        }
        /* pop returned value */
        lua_pop(L, 1);
    }

    return ret;
}

int lua_call_request(nproxy_connection_t * conn) {
    lua_State *L = NULL;
    int ret = HTTP_INTERNAL_SERVER_ERROR;

    assert(conn);

    lua_connection_start(conn);
    if (!(L = conn->lua)) {
        return ret;
    }

    /* push functions and arguments */
    lua_getglobal(L, "on_request"); /* function to be called */

    lua_push_conn(L, conn);

    /* do the call (2 arguments, 1 result) */
    if (lua_pcall(L, 1, 1, 0) != 0) {
        nn_log(NN_LOG_ERROR, "error running function `%s': %s", "on_request", lua_tostring(L, -1));
        /* pop returned value */
        lua_pop(L, 1);
    } else {
        /* retrieve result */
        if (!lua_isnumber(L, -1)) {
            nn_log(NN_LOG_ERROR, "function `on_request' must return a number");
        } else {
            ret = lua_tonumber(L, -1);
        }
        /* pop returned value */
        lua_pop(L, 1);
    }

    return ret;
}

int lua_call_response(nproxy_connection_t * conn) {
    lua_State *L = NULL;
    int ret = HTTP_INTERNAL_SERVER_ERROR;
    int code = 0;

    assert(conn);

    lua_connection_start(conn);
    if (!(L = conn->lua)) {
        return ret;
    }

    /* push functions and arguments */
    lua_getglobal(L, "on_response");    /* function to be called */

    lua_push_conn(L, conn);
    lua_pushnumber(L, code);    /* push 2nd argument */

    /* do the call (2 arguments, 1 result) */
    if (lua_pcall(L, 2, 1, 0) != 0) {
        nn_log(NN_LOG_ERROR, "error running function `%s': %s", "on_response", lua_tostring(L, -1));
        /* pop returned value */
        lua_pop(L, 1);
    } else {
        /* retrieve result */
        if (!lua_isnumber(L, -1)) {
            nn_log(NN_LOG_ERROR, "function `on_response' must return a number");
        } else {
            ret = lua_tonumber(L, -1);
        }
        /* pop returned value */
        lua_pop(L, 1);
    }

    return ret;
}

int lua_call_log(nproxy_connection_t * conn) {
    lua_State *L = NULL;
    int ret = HTTP_INTERNAL_SERVER_ERROR;

    assert(conn);

    lua_connection_start(conn);
    if (!(L = conn->lua)) {
        return ret;
    }

    /* push functions and arguments */
    lua_getglobal(L, "on_log_request"); /* function to be called */

    lua_push_conn(L, conn);

    /* do the call (1 arguments, 1 result) */
    if (lua_pcall(L, 1, 1, 0) != 0) {
        nn_log(NN_LOG_ERROR, "error running function `%s': %s", "on_log_request", lua_tostring(L, -1));
        /* pop returned value */
        lua_pop(L, 1);
    } else {
        /* retrieve result */
        if (!lua_isnumber(L, -1)) {
            nn_log(NN_LOG_ERROR, "function `on_log_request' must return a number");
        } else {
            ret = lua_tonumber(L, -1);
        }
        /* pop returned value */
        lua_pop(L, 1);
    }

    return ret;
}

int lua_call_auth_basic(nproxy_connection_t * conn, const char *type, const char *data) {
    lua_State *L = NULL;
    int ret = HTTP_INTERNAL_SERVER_ERROR;

    assert(conn);

    lua_connection_start(conn);
    if (!(L = conn->lua)) {
        return ret;
    }

    /* push functions and arguments */
    lua_getglobal(L, "on_auth_basic");  /* function to be called */

    lua_push_conn(L, conn);
    lua_pushstring(L, type);    /* push 2st argument */
    lua_pushstring(L, data);    /* push 3st argument */

    /* do the call (3 arguments, 1 result) */
    if (lua_pcall(L, 3, 1, 0) != 0) {
        nn_log(NN_LOG_ERROR, "error running function `%s': %s", "on_auth_basic", lua_tostring(L, -1));
        /* pop returned value */
        lua_pop(L, 1);
    } else {
        /* retrieve result */
        if (!lua_isnumber(L, -1)) {
            nn_log(NN_LOG_ERROR, "function `on_auth_basic' must return a number");
        } else {
            ret = lua_tonumber(L, -1);
        }
        /* pop returned value */
        lua_pop(L, 1);
    }

    return ret;
}
