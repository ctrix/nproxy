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
 * libnproxy_swig.c
 *
 */

#include "libnproxy.h"
#include "libnproxy_swig.h"

void logmsg(const char *how, const char *msg) {
    int lev;

    if (!strncasecmp(how, "verbose", strlen("verbose"))) {
        lev = NN_LOG_VERBOSE;
    } else if (!strncasecmp(how, "error", strlen("error"))) {
        lev = NN_LOG_ERROR;
    } else if (!strncasecmp(how, "warning", strlen("warning"))) {
        lev = NN_LOG_WARNING;
    } else if (!strncasecmp(how, "notice", strlen("notice"))) {
        lev = NN_LOG_NOTICE;
    } else if (!strncasecmp(how, "debug", strlen("debug"))) {
        lev = NN_LOG_DEBUG;
    } else {
        lev = NN_LOG_CRIT;
    }
    nn_log(lev, "%s", msg);

    return;
}

char *profile_get_script_dir(nproxy_connection_t * conn) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return NULL;
    }

    if (conn->profile) {
        return conn->profile->script_dir;
    }
    return NULL;
}

void connection_set_authuser(nproxy_connection_t * conn, const char *name) {
    if (conn && !zstr(name)) {
        conn->auth_user = apr_pstrdup(conn->pool, name);
    }
    return;
}

char *connection_get_authuser(nproxy_connection_t * conn) {
    if (conn != NULL) {
        return conn->auth_user;
    } else {
        return NULL;
    }
}

void connection_set_variable(nproxy_connection_t * conn, const char *name, const char *value) {
    nproxy_connection_set_variable(conn, name, value);
    return;
}

const char *connection_get_variable(nproxy_connection_t * conn, const char *name) {
    return nproxy_connection_get_variable(conn, name);
}

void connection_set_inactivity_timeout(nproxy_connection_t * conn, int secs) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    nn_log(NN_LOG_DEBUG, "Setting inactivity timeout to %ds", secs);
    conn->inactivity_timeout = secs;
    return;
}

void connection_set_max_duration(nproxy_connection_t * conn, int secs) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    nn_log(NN_LOG_DEBUG, "Setting max duration to %ds", secs);
    conn->max_duration = secs;
    return;
}

void connection_set_traffic_shaper(nproxy_connection_t * conn, size_t unshaped_bytes, size_t bytes_per_sec) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    nn_log(NN_LOG_DEBUG, "Shaping connection to %zubps after initial burst of %zu bytes", bytes_per_sec, unshaped_bytes);
    conn->limit_max_size = unshaped_bytes;
    conn->limit_bps = bytes_per_sec;

    if (conn->requests) {
        conn->requests->limit_max_size = unshaped_bytes;
        conn->requests->limit_bps = bytes_per_sec;
    }
    return;
}

void request_force_upstream(nproxy_connection_t * conn, const char *host, int port) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (!conn->requests) {
        return;
    }

    if (zstr(host) || !port || (port < 0)) {
        return;
    }

    nn_log(NN_LOG_DEBUG, "Forcing upstream to %s:%d", host, port);

    conn->requests->force_host = apr_pstrdup(conn->pool, host);
    conn->requests->force_port = port;

    return;
}

void request_force_bind(nproxy_connection_t * conn, const char *host) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (!conn->requests) {
        return;
    }

    if (zstr(host)) {
        return;
    }

    nn_log(NN_LOG_DEBUG, "Forcing bind address to %s", host);

    conn->requests->force_bind = apr_pstrdup(conn->pool, host);

    return;
}

void request_require_auth(nproxy_connection_t * conn, char *type, const char *realm) {

    if (conn->requests && type && realm && strlen(realm)) {
        conn->requests->require_auth = 1;
        conn->requests->auth_realm = apr_pstrdup(conn->pool, realm);
    }

    return;
}

void request_set_traffic_limit(nproxy_connection_t * conn, size_t in, size_t out) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    nn_log(NN_LOG_DEBUG, "Setting request max traffic limits. In: %zu Out: %zu", in, out);
    if (conn->requests) {
        conn->requests->max_in = in;
        conn->requests->max_out = out;
    }

    return;
}

void request_set_traffic_shaper(nproxy_connection_t * conn, size_t unshaped_bytes, size_t bytes_per_sec) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    nn_log(NN_LOG_DEBUG, "Shaping request to %zubps after initial burst of %zu bytes", bytes_per_sec, unshaped_bytes);
    conn->requests->limit_max_size = unshaped_bytes;
    conn->requests->limit_bps = bytes_per_sec;

    return;
}

void request_change_url(nproxy_connection_t * conn, const char *url) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(url)) {
        url = "/";
    }

    conn->requests->url = apr_pstrdup(conn->pool, url);

    return;
}

char *request_get_header(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return NULL;
    }

    if (zstr(h)) {
        return NULL;
    }

    return find_header(&conn->requests->request_headers, h);
}

char *request_get_header_next(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return NULL;
    }

    if (zstr(h)) {
        return NULL;
    }

    return find_header_next(&conn->requests->request_headers, h);
}

void request_del_header(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(h)) {
        return;
    }

    clear_header(&conn->requests->request_headers, h);
    return;
}

void request_del_header_current(nproxy_connection_t * conn) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    clear_header_current(&conn->requests->request_headers);
    return;
}

void request_add_header(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(h)) {
        return;
    }

    add_header(conn, &conn->requests->request_headers, h);
    return;
}

void request_replace_header_current(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(h)) {
        return;
    }

    replace_header_current(conn, &conn->requests->request_headers, h);
    return;
}

int request_is_transparent(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->is_transparent;
    }
    return 0;
}

char *request_get_host(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->host;
    }
    return NULL;
}

int request_get_port(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->port;
    }
    return 0;
}

char *request_get_method(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->method;
    }
    return NULL;
}

char *request_get_url(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->url;
    }
    return NULL;
}

int request_get_protocol(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->request_headers.proto;
    }
    return -1;
}

char *response_get_header(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return NULL;
    }

    if (zstr(h)) {
        return NULL;
    }

    return find_header(&conn->requests->response_headers, h);
}

char *response_get_header_next(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return NULL;
    }

    if (zstr(h)) {
        return NULL;
    }

    return find_header_next(&conn->requests->response_headers, h);
}

void response_del_header(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(h)) {
        return;
    }

    clear_header(&conn->requests->response_headers, h);
    return;
}

void response_del_header_current(nproxy_connection_t * conn) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    clear_header_current(&conn->requests->response_headers);
    return;
}

void response_add_header(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(h)) {
        return;
    }

    add_header(conn, &conn->requests->response_headers, h);
    return;
}

void response_replace_header_current(nproxy_connection_t * conn, const char *h) {
    if (conn == NULL) {
        nn_log(NN_LOG_ERROR, "%s: null connection", __FUNCTION__);
        return;
    }

    if (zstr(h)) {
        return;
    }

    replace_header_current(conn, &conn->requests->response_headers, h);
    return;
}

int response_get_code(nproxy_connection_t * conn) {
    if (conn->requests) {
        return conn->requests->response_headers.resp_code;
    } else {
        return -1;
    }
}
