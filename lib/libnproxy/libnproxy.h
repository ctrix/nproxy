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
 * libnproxy.h
 *
 */

#ifndef LIB_NPROXY_H
#define LIB_NPROXY_H

#define DEBUG_CONNECTION 0

#if DEBUG_CONNECTION > 0
#define APR_POOL_DEBUG TRUE
#endif

#ifdef WIN32 
#define snprintf sprintf_s
#endif

#include <assert.h>
#include <stdlib.h>

#include "apr.h"
#include "apr_file_io.h"
#include "apr_hash.h"
#include "apr_network_io.h"
#include "apr_poll.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_time.h"
#include "apr_thread_proc.h"

#include "nn_log.h"
#include "nn_utils.h"
#include "nn_buffer.h"

#include "nproxy.h"

#define DEFAULT_SOCKET_BACKLOG SOMAXCONN
#define DEFAULT_POLLSET_NUM 4
#define DEFAULT_POLL_TIMEOUT (APR_USEC_PER_SEC * 1)
#define DEFAULT_THREAD_STACKSIZE 250 * 1024
#define DEFAULT_MAX_CLIENTS 100
#define DEFAULT_BUFFER_SIZE 8192

#define HTTP_PORT 80

#define CONNECT_RESPONSE "HTTP/1.0 200 Connection established\r\n"
#define PROXY_AGENT "Proxy-agent: NProxy\r\n"

typedef enum http_protocols_e http_protocols_t;
enum http_protocols_e {
    HTTP_09 = 0,                /* Note that this is duplicated in the swig.i file */
    HTTP_10 = 1,                /* Note that this is duplicated in the swig.i file */
    HTTP_11 = 2,                /* Note that this is duplicated in the swig.i file */
};

typedef enum side_e side_t;
enum side_e {
    SIDE_NONE = 0,
    SIDE_BROWSER = 4,
    SIDE_SERVER = 5
};

typedef enum http_resp_codes_e http_resp_codes_t;
enum http_resp_codes_e {
    HTTP_CONTINUE = 100,
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_MULTIPLE_CH = 300,
    HTTP_MOVED_PERM = 301,
    HTTP_MOVED_TEMP = 302,
    HTTP_NOT_MODIFIED = 304,
    HTTP_USE_PROXY = 305,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_PROXY_AUTH_REQ = 407,
    HTTP_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVALAIBLE = 503,
    HTTP_VERSION_NOT_SUPPORTED = 505,
};

typedef struct nproxy_request_headers_s nproxy_request_headers_t;
struct nproxy_request_headers_s {
    char *raw;
    apr_size_t rawlen;
    apr_size_t count;
    char **lines;
    apr_size_t last_script_access;

    http_protocols_t proto;
    int resp_code;
    char is_chunked;
    apr_size_t chunk_len;
    apr_size_t chunk_sent;
    apr_size_t content_sent;
    apr_ssize_t content_len;
    char must_close;
    char headers_sent;
    char document_completed;
};

typedef struct nproxy_request_s nproxy_request_t;
struct nproxy_request_s {
    apr_time_t created;
    apr_time_t started;
    char logged;
    char terminated;
    char is_transparent;

    char *force_bind;
    char *force_host;
    apr_uint16_t force_port;
    char *host;
    apr_uint16_t port;
    char *method;
    char *url;
    char *protocol;
    char *path;

    char *resp_text;
    int resp_code;

    nproxy_request_headers_t request_headers;
    nproxy_request_headers_t response_headers;

    char is_connect;
    char require_auth;
    char *auth_realm;

    apr_time_t limit_started;
    size_t limit_max_size;
    size_t limit_bps;

    apr_uint32_t bytes_in;
    apr_uint32_t bytes_out;

    apr_uint32_t max_in;
    apr_uint32_t max_out;

    nproxy_request_t *next;
};

typedef struct nproxy_connection_side_s nproxy_connection_side_t;
struct nproxy_connection_side_s {
    nn_buffer_t *buffer;
    apr_socket_t *sock;
    apr_pollfd_t pfd;

    struct {
        char *host;
        apr_uint16_t port;
        char is_connect;
    } server;
};

struct nproxy_connection_s {
    apr_pool_t *pool;

    nproxy_profile_t *profile;
    char *auth_user;

    char terminated;

    nproxy_connection_side_t *browser;
    nproxy_connection_side_t *server;
    nproxy_request_t *requests;
    nproxy_request_t *requests_tail;
    apr_hash_t *var_hash;
    void *lua;

    int timer_interval;
    int inactivity_timeout;
    int max_duration;
    apr_time_t started;
    apr_time_t last_io;

    apr_time_t limit_started;
    size_t limit_max_size;
    size_t limit_bps;

    /* Runtime */
    apr_socket_t *sock;
    apr_pollset_t *pollset;
    char *read_buffer;
    size_t read_buffer_size;

    apr_uint32_t bytes_in;
    apr_uint32_t bytes_out;
};

struct nproxy_profile_stats_s {
    unsigned long int num_reqs;
    unsigned long int num_badcons;
    unsigned long int num_open;
    unsigned long int num_refused;
    unsigned long int num_denied;
};

typedef enum {
    NP_PROFILE_ALLOW_CONNECT = (1 << 0),
    NP_PROFILE_ALLOW_TRANSPARENT = (1 << 1),
    NP_PROFILE_SKIP_CONNECT_ACL = (1 << 2),
    NP_PROFILE_SKIP_REQUEST_ACL = (1 << 3),
    NP_PROFILE_SKIP_RESPONSE_ACL = (1 << 4),
    NP_PROFILE_SKIP_AUTH_ACL = (1 << 5),
} profile_flags_t;

struct nproxy_profile_s {
    char *name;

    char *listen_address;
    int listen_port;

    char *bind_address;
    char disable_ipv4;
    char disable_ipv6;

    int inactivity_timeout;
    int max_duration;

    char *template_dir;
    char *script_dir;
    char *luascript;

    size_t limit_max_size;
    size_t limit_bps;

    int max_clients;
    apr_uint32_t flags;

    /* Runtime */
    apr_pool_t *pool;
    apr_socket_t *sock;
    char running;
    char stopped;
    apr_pollset_t *pollset;

    // TODO stats ?
};

nproxy_connection_t *nproxy_connection_create(nproxy_profile_t * profile, apr_socket_t * sock, apr_pool_t * pool);
const char *nproxy_connection_get_variable(nproxy_connection_t * conn, const char *name);
apr_status_t nproxy_connection_set_variable(nproxy_connection_t * conn, const char *varname, const char *varvalue);
apr_status_t nproxy_connection_play(nproxy_connection_t * conn);
void nproxy_connection_destroy(nproxy_connection_t * conn);

apr_status_t http_send_custom_response(nproxy_connection_t * conn, http_resp_codes_t error_code);

/* UTILS */
apr_status_t chomp(char *buffer, size_t length);
size_t extract_chunk_size(nn_buffer_t * buf, int *action);
apr_socket_t *prepare_outgoing_socket(nproxy_connection_t * conn, const char *host, int port, const char *bind_to);

/* HEADERS */
char *find_header(nproxy_request_headers_t * headers, const char *header);
char *find_header_next(nproxy_request_headers_t * headers, const char *header);
void clear_header(nproxy_request_headers_t * headers, const char *header);
void clear_header_current(nproxy_request_headers_t * headers);
void add_header(nproxy_connection_t * conn, nproxy_request_headers_t * headers, const char *text);
void replace_header_current(nproxy_connection_t * conn, nproxy_request_headers_t * headers, const char *text);

/* AUTH */
char *base64_decode(apr_pool_t * pool, const char *in);
int basic_challenge_auth(nproxy_connection_t * conn, const char *digest);

#ifdef LUA_VERSION_NUMBER
void lua_connection_stop(nproxy_connection_t * conn);
int lua_call_connect(nproxy_connection_t * conn, char *ip);
int lua_call_request(nproxy_connection_t * conn);
int lua_call_response(nproxy_connection_t * conn);
int lua_call_log(nproxy_connection_t * conn);
int lua_call_auth_basic(nproxy_connection_t * conn, const char *type, const char *data);
#include "libnproxy_swig.h"
#endif

#endif
