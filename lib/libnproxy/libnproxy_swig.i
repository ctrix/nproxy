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
 * libnproxy_swig.i
 *
 */

/* "USE swig -lua libnproxy_swig.i to RESWIG" */

% module nproxy % {
#include "nproxy.h"
#include "libnproxy_swig.h"
%}

%wrapper % {
    void lua_push_conn(lua_State * L, nproxy_connection_t * conn) {
        SWIG_NewPointerObj(L, conn, SWIGTYPE_p_nproxy_connection_t, 0);
    }
%}

%luacode {
HTTP_09 = 0 HTTP_10 = 1 HTTP_11 = 2}

void logmsg(const char *how, const char *msg);

char *profile_get_script_dir(nproxy_connection_t * conn);

void connection_prefer_ipv4(nproxy_connection_t * conn);
void connection_prefer_ipv6(nproxy_connection_t * conn);

void connection_set_authuser(nproxy_connection_t * conn, const char *name);
char *connection_get_authuser(nproxy_connection_t * conn);
void connection_set_variable(nproxy_connection_t * conn, const char *name, const char *value);
const char *connection_get_variable(nproxy_connection_t * conn, const char *name);

void connection_set_inactivity_timeout(nproxy_connection_t * conn, int secs);
void connection_set_max_duration(nproxy_connection_t * conn, int secs);
void connection_set_traffic_shaper(nproxy_connection_t * conn, size_t unshaped_bytes, size_t bytes_per_sec);

void request_force_upstream(nproxy_connection_t * conn, const char *host, int port);
void request_force_bind(nproxy_connection_t * conn, const char *host);
void request_require_auth(nproxy_connection_t * conn, char *type, const char *realm);
void request_set_traffic_limit(nproxy_connection_t * conn, size_t in, size_t out);
void request_set_traffic_shaper(nproxy_connection_t * conn, size_t unshaped_bytes, size_t bytes_per_sec);

void request_change_url(nproxy_connection_t * conn, const char *url);
char *request_get_header(nproxy_connection_t * conn, const char *h);
char *request_get_header_next(nproxy_connection_t * conn, const char *h);
void request_del_header(nproxy_connection_t * conn, const char *h);
void request_del_header_current(nproxy_connection_t * conn);
void request_add_header(nproxy_connection_t * conn, const char *h);
void request_replace_header_current(nproxy_connection_t * conn, const char *h);

int request_is_transparent(nproxy_connection_t * conn);
char *request_get_host(nproxy_connection_t * conn);
int request_get_port(nproxy_connection_t * conn);
char *request_get_method(nproxy_connection_t * conn);
char *request_get_url(nproxy_connection_t * conn);
int request_get_protocol(nproxy_connection_t * conn);

char *response_get_header(nproxy_connection_t * conn, const char *h);
char *response_get_header_next(nproxy_connection_t * conn, const char *h);
void response_del_header(nproxy_connection_t * conn, const char *h);
void response_del_header_current(nproxy_connection_t * conn);
void response_add_header(nproxy_connection_t * conn, const char *h);
void response_replace_header_current(nproxy_connection_t * conn, const char *h);
int response_get_code(nproxy_connection_t * conn);
