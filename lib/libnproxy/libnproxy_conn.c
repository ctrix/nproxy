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
 * libnproxy_conn.c
 *
 */

#include "libnproxy.h"

static apr_status_t server_force_disconnect(nproxy_connection_t * conn);
static void on_client_disconnect(nproxy_connection_t * conn);

/* ************************************************************************* */

const char *nproxy_connection_get_variable(nproxy_connection_t * conn, const char *name) {
    if (!conn->var_hash) {
        return NULL;
    }

    if (zstr(name)) {
        return NULL;
    }

    return apr_hash_get(conn->var_hash, name, APR_HASH_KEY_STRING);
}

apr_status_t nproxy_connection_set_variable(nproxy_connection_t * conn, const char *varname, const char *varvalue) {
    char *value = NULL;

    if (!conn->var_hash) {
        conn->var_hash = apr_hash_make(conn->pool);
        if (!conn->var_hash) {
            return APR_STATUS_ERROR;
        }
    }

    value = apr_pstrdup(conn->pool, varvalue);

#if DEBUG_CONNECTION >= 1
    nn_log(NN_LOG_DEBUG, "Setting variable '%s' with value '%s'", varname, value ? value : "[empty]");
#endif

    if (!zstr(varname)) {
        if (!zstr(value)) {
            apr_hash_set(conn->var_hash, varname, APR_HASH_KEY_STRING, value);
        } else {
            apr_hash_set(conn->var_hash, varname, APR_HASH_KEY_STRING, NULL);
        }
    }

    return APR_STATUS_SUCCESS;
}

/* ************************************************************************* */

static nproxy_request_t *request_create(nproxy_connection_t * conn) {
    nproxy_request_t *r;

    r = apr_pcalloc(conn->pool, sizeof(nproxy_request_t));
    if (r != NULL) {
        r->limit_max_size = conn->limit_max_size;
        r->limit_bps = conn->limit_bps;
        r->created = apr_time_now();
    }
#if DEBUG_CONNECTION >= 3
    nn_log(NN_LOG_DEBUG, "New request created");
#endif

    return r;
}

static void request_log(nproxy_connection_t * conn) {
    nproxy_request_t *req;
    nproxy_request_headers_t *reqh;
    nproxy_request_headers_t *resh;

    req = conn->requests;
    if (!req) {
        return;
    }

    reqh = &conn->requests->request_headers;
    resh = &conn->requests->response_headers;

    /* Avoid logging invalid requests */
    if (!resh->resp_code && !req->method && !req->host) {
        return;
    }

    reqh->last_script_access = 0;
    resh->last_script_access = 0;

    if (!req->logged) {
        apr_sockaddr_t *sa;
        char *ip = NULL;
        char *authuser = NULL;
        int code = 0;
        double dur = 0;

        req->logged = 1;

        if (lua_call_log(conn) == -1) {
            return;
        }

        dur = (double) (apr_time_as_msec(apr_time_now() - req->started)) / 1000.0;

        apr_socket_addr_get(&sa, APR_REMOTE, conn->sock);
        apr_sockaddr_ip_get(&ip, sa);

        code = resh->resp_code;
        authuser = conn->auth_user;

        if (req->port != 80) {
            nn_log(NN_LOG_NOTICE, "%s %s %s %s:%d \"%s\" %s %d %.2f %d %d",
                   ip, (authuser) ? authuser : "-", req->method, req->host, req->port, req->url, req->protocol, (code != 0) ? code : -1, dur, req->bytes_in, req->bytes_out);
        } else {
            nn_log(NN_LOG_NOTICE, "%s %s %s %s \"%s\" %s %d %.2f %d %d",
                   ip, (authuser) ? authuser : "-", req->method, req->host, req->url, req->protocol, (code != 0) ? code : -1, dur, req->bytes_in, req->bytes_out);
        }
    }

    return;
}

static void request_purge(nproxy_connection_t * conn) {
    nproxy_request_t *req;

    req = conn->requests;
    if (!req) {
        return;
    }

    request_log(conn);

    conn->requests = conn->requests->next;
    if (!conn->requests) {
        conn->requests_tail = NULL;
    }

    if (req->response_headers.must_close) {
        on_client_disconnect(conn);
    } else if (req->request_headers.must_close) {
        on_client_disconnect(conn);
    }

    return;
}

/* ************************************************************************* */

static apr_status_t send_headers(apr_socket_t * sock, size_t lines, char **headers, char *symbols) {
    size_t t;
    char *h = NULL;
    apr_size_t len;
    apr_status_t ret = APR_STATUS_ERROR;

    assert(symbols);

    for (t = 0; t < lines; t++) {
        h = headers[t];
        if (h == NULL) {
            continue;
        }

        if (strlen(h) > 0) {
            len = strlen(h);
            ret = apr_socket_send(sock, h, &len);
            len = 2;
            ret = apr_socket_send(sock, "\r\n", &len);
        }
    }

    len = 2;
    ret = apr_socket_send(sock, "\r\n", &len);

    return ret;
}

static apr_status_t request_send_response_headers(nproxy_connection_t * conn, nproxy_request_t * req) {

    if (!req->response_headers.lines) {
        assert(0);
        return APR_STATUS_ERROR;
    }

    return send_headers(conn->browser->sock, req->response_headers.count, req->response_headers.lines, "<<");
}

static apr_status_t request_send_request_headers(nproxy_connection_t * conn, nproxy_request_t * req) {
    apr_size_t len = 0;
    char *line = NULL;
    apr_ssize_t ret = 0;

    (void) ret;

    /* TODO
       Remove connection headers, eventually, Add specific headers , Add VIA headers, Delete the headers that we're asked to remove 
     */

    if (!req->method || !req->url || !req->protocol) {
        return APR_STATUS_ERROR;
    }

    len = strlen(req->method) + 1 + strlen(req->url) + 1 + strlen(req->protocol) + 1;
    line = apr_pcalloc(conn->pool, len);
    if (!line) {
        return APR_STATUS_ERROR;
    }

    req->started = apr_time_now();

    snprintf(line, len, "%s %s %s", req->method, req->url, req->protocol);
    chomp(line, strlen(line));

    len = strlen(line);
    ret = apr_socket_send(conn->server->sock, line, &len);

    len = 2;
    ret = apr_socket_send(conn->server->sock, "\r\n", &len);

    if (!req->request_headers.lines) {
        //assert(0);
        return APR_STATUS_ERROR;
    }

    clear_header(&req->request_headers, "Proxy-Authorization");

    req->request_headers.lines[0] = NULL;

    return send_headers(conn->server->sock, req->request_headers.count, req->request_headers.lines, ">>");
}

/* ************************************************************************* */

static apr_status_t parse_server_headers(nproxy_connection_t * conn, nproxy_request_t * req) {
    apr_size_t t;
    size_t ret;
    char *request = NULL;
    char protocol[10];
    unsigned int code;
    nproxy_request_headers_t *headers;

    headers = &req->response_headers;

    headers->is_chunked = 0;
    headers->content_len = -1;
    headers->must_close = 0;

    if (!headers->lines || !headers->count) {
        //assert(0);
        return APR_STATUS_ERROR;
    }

    if (!(request = apr_pstrdup(conn->pool, headers->lines[0]))) {
        return APR_STATUS_ERROR;
    }

    if (request) {
        chomp(request, strlen(request));
    }

    ret = sscanf(request, "%[^ ] %u %*s", protocol, &code);
    if (ret != 2) {
        return APR_STATUS_ERROR;
    } else {
        unsigned int major, minor;

        /* Break apart the protocol and update the connection structure. */
        /* If the conversion doesn't succeed, drop down below and send the error to the user. */
        ret = sscanf(protocol + 5, "%u.%u", &major, &minor);
        if (ret != 2) {
            return APR_STATUS_ERROR;
        }
        if (major != 1) {
            return APR_STATUS_ERROR;
        }
        if (minor == 0) {
            headers->must_close = 1;
            headers->proto = HTTP_10;
        } else if (minor == 1) {
            headers->proto = HTTP_11;
        } else {
            return APR_STATUS_ERROR;
        }

        headers->resp_code = code;

#if DEBUG_CONNECTION >= 5
        nn_log(NN_LOG_DEBUG, "server proto: %d code: %d", headers->proto, headers->resp_code);
#endif
    }

#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_DEBUG, "server headers count: %zu", headers->count);
#endif

    for (t = 0; t < headers->count; t++) {
        char *h = NULL;
        h = headers->lines[t];

        if (!h) {
            continue;
        }

        chomp(h, strlen(h));

        if (!strncasecmp(h, "Content-Length:", strlen("Content-Length:"))) {
            char *cl = NULL;
            cl = h + strlen("Content-Length:");
            headers->content_len = atoi(cl);
#if DEBUG_CONNECTION >= 3
            nn_log(NN_LOG_DEBUG, "server content len: %zu", headers->content_len);
#endif
        } else if (!strncasecmp(h, "Transfer-Encoding:", strlen("Transfer-Encoding:"))) {
            if (strcasestr(h, "chunked")) {
                headers->is_chunked = 1;
            }
        } else if (!strncasecmp(h, "Connection:", strlen("Connection:"))) {
            if (strcasestr(h, "close")) {
                headers->must_close = 1;
#if DEBUG_CONNECTION >= 5
                nn_log(NN_LOG_DEBUG, "** server must close");
#endif
            }
            headers->lines[t] = NULL;
        }

    }

    if (((headers->resp_code >= 100 && headers->resp_code <= 199) || headers->resp_code == 204 || headers->resp_code == 205 || headers->resp_code == 304)
        ) {
        headers->must_close = 1;
    }

    if ((headers->resp_code == 200 && headers->proto == HTTP_11 && !headers->must_close && !headers->is_chunked && (headers->content_len < 0)
        )
        ) {
        /*
           Upstream is buggy.
           an HTTP/1.1 response with a 200 status code doesn't have a content len, is not chuncked and doesn't require a connection: close
         */
        nn_log(NN_LOG_NOTICE, "Upstream bug detected on %d response", headers->resp_code);
        headers->must_close = 1;
    }

    /* Restore the default value of content len */
    if (headers->content_len == -1) {
#if DEBUG_CONNECTION >= 3
        nn_log(NN_LOG_DEBUG, "Restoring server content len to zero");
#endif
        headers->content_len = 0;
    }

    return APR_STATUS_SUCCESS;
}

static apr_status_t parse_client_headers(nproxy_connection_t * conn, nproxy_request_t * req) {
    apr_size_t t;
    size_t ret;
    char *request = NULL;
    nproxy_request_headers_t *headers;

    headers = &req->request_headers;

    if (!headers->count || !headers->lines) {
        return APR_STATUS_ERROR;
    }

    headers->is_chunked = 0;
    headers->content_len = -1;
    headers->must_close = 0;

    if (!(request = apr_pstrdup(conn->pool, headers->lines[0]))) {
        return APR_STATUS_ERROR;
    }
    if (request) {
        chomp(request, strlen(request));
    }
#if DEBUG_CONNECTION >= 3
    nn_log(NN_LOG_DEBUG, "client headers count: %zu len: %zu", headers->count, headers->rawlen);
#endif

    /* Parsing the request line */
    req->method = apr_pcalloc(conn->pool, strlen(request) + 1);
    req->url = apr_pcalloc(conn->pool, strlen(request) + 1);
    req->protocol = apr_pcalloc(conn->pool, strlen(request) + 1);

    ret = sscanf(request, "%[^ ] %[^ ] %[^ ]", req->method, req->url, req->protocol);

    if (ret == 2 && !strcasecmp(req->method, "GET")) {
        /* Indicate that this is a HTTP/0.9 GET request */
        headers->proto = HTTP_09;
    } else if (ret == 3 && !strncasecmp(req->protocol, "HTTP/", 5)) {
        unsigned int major, minor;
        /* Break apart the protocol and update the connection structure. */
        ret = sscanf(req->protocol + 5, "%u.%u", &major, &minor);

        /* If the conversion doesn't succeed, drop down below and send the error to the user. */
        if (ret != 2) {
            return APR_STATUS_ERROR;
        }
        if (major != 1) {
            return APR_STATUS_ERROR;
        }
        if (minor == 0) {
            headers->proto = HTTP_10;
        } else if (minor == 1) {
            headers->proto = HTTP_11;
        } else {
            return APR_STATUS_ERROR;
        }
    } else {
        /* Bad request */
        return APR_STATUS_ERROR;
    }

    if (!req->url) {
        return APR_STATUS_ERROR;
    }

    if (headers->proto == HTTP_10) {
        headers->must_close = 1;
    } else if (!strncmp(req->method, "HEAD", 5)) {
        headers->must_close = 1;
    } else {
        headers->must_close = 0;
    }

    /* TODO Reverse support */

    /* Analyze the type of request */
    req->host = apr_pcalloc(conn->pool, strlen(req->url) + 1);
    req->path = apr_pcalloc(conn->pool, strlen(req->url) + 1);
    req->is_connect = 0;

    if (!strncasecmp(req->url, "http://", 7) /* TODO ?? || !strncasecmp(req->url, "ftp://", 6) */ ) {
        char *c = NULL;
        char *skipped_type = strstr(req->url, "//") + 2;

        if (sscanf(skipped_type, "[%[:a-fA-F0-9]]:%hu", req->host, &req->port) == 2) {
            /* Ok, we have an IPv6 host and a port, eventually */
        } else if (sscanf(skipped_type, "[%[:a-fA-F0-9]]", req->host) == 1) {
            /* Ok, we have an IPv6 host and a port, eventually */
            req->port = HTTP_PORT;
        } else if (sscanf(skipped_type, "%[^:/]:%hu", req->host, &req->port) == 2) {
            /* Ok, we have a port and a host */
        } else if (sscanf(skipped_type, "%s", req->host) == 1) {
            /* We only have a port */
            req->port = HTTP_PORT;
        } else {
            /*400, "Bad Request" */
            return APR_STATUS_ERROR;
        }

        if ((c = strchr(req->host, '/')) != NULL) {
            *c = '\0';
        }
        if ((c = strchr(req->host, '?')) != NULL) {
            *c = '\0';
        }

        skipped_type = strstr(req->url, "//") + 2;
        if ((c = strchr(skipped_type, '/')) != NULL) {
            req->url = c;
        } else {
            req->url = apr_pstrdup(conn->pool, "/Uh?");
        }
    } else if (!strcmp(req->method, "CONNECT")) {
        char *c = NULL;
        if (sscanf(req->url, "%[^:]:%hu", req->host, &req->port) == 2) {
        } else if (sscanf(req->url, "%s", req->host) == 1) {
            //400, "Bad Request"
            return APR_STATUS_ERROR;
        } else {
            //400, "Bad Request"
            return APR_STATUS_ERROR;
        }

        if ((c = strchr(req->host, '/')) != NULL) {
            *c = '\0';
        }
        if ((c = strchr(req->host, '?')) != NULL) {
            *c = '\0';
        }

        req->is_connect = 1;
    } else {
        /* Transparent proxy */
        apr_size_t t;
        char *c = NULL;
        char *host = NULL;

        for (t = 0; t < headers->count; t++) {
            if (headers->lines[t] && !strncmp(headers->lines[t], "Host:", 5)) {
                host = headers->lines[t] + 5;
                break;
            }
        }

        if (!host) {
            nn_log(NN_LOG_ERROR, "No Host: line in the headers");
            return APR_STATUS_ERROR;
        }

        req->host = apr_pstrdup(conn->pool, host);  /* We can't reuse the conn->host already allocated. It may be too small. */
        while (req->host && *req->host == ' ') {
            req->host++;
        }
        chomp(req->host, strlen(req->host));

        /* If we have a port in the host header, then use it */
        if ((c = strchr(req->host, ':'))) {
            *c = '\0';
            req->port = atoi(c + 1);
            if (!req->port) {
                return APR_STATUS_ERROR;
            }
        } else {
            /* TODO Check that the host doesn't contain the port */
            req->port = HTTP_PORT;
        }

        if (!host) {
            return APR_STATUS_ERROR;
        }

        req->is_transparent = 1;
    }

/*
#if DEBUG_CONNECTION >= 5
    if (req->port != 80) {
        nn_log(NN_LOG_DEBUG, "%s (%d) - %s %s - %s", req->host, req->port, req->protocol, req->method, req->url);
    } else {
        nn_log(NN_LOG_DEBUG, "%s - %s %s - %s", req->host, req->protocol, req->method, req->url);
    }
#endif
*/

    /* Analyze some headers */
    for (t = 1; t < headers->count; t++) {
        char *h = NULL;
        h = headers->lines[t];

        if (!h) {
            continue;
        }

        chomp(h, strlen(h));

        if (!strncasecmp(h, "Proxy-Connection", strlen("Proxy-Connection"))) {
            headers->lines[t] = NULL;
            continue;
        }

        else if (!strncmp(h, "Connection", strlen("Connection"))) {
            if (strcasestr(h, "close")) {
                headers->must_close = 1;
#if DEBUG_CONNECTION >= 5
                nn_log(NN_LOG_DEBUG, "*** client should close");
#endif
            }
            headers->lines[t] = NULL;
            continue;
        } else if (!strncasecmp(h, "Content-Length:", strlen("Content-Length:"))) {
            char *cl = NULL;
            cl = h + strlen("Content-Length:");
            headers->content_len = atoi(cl);
        } else if (!strncasecmp(h, "Transfer-Encoding:", strlen("Transfer-Encoding:"))) {
            if (strcasestr(h, "chunked")) {
                headers->is_chunked = 1;
            }
        }
#if DEBUG_CONNECTION >= 11
        /* This is for debug purposes only, to avoid gzipped stuff */
        else if (!strncasecmp(h, "Accept-Encoding", strlen("Accept-Encoding"))) {
            headers->lines[t] = NULL;
            continue;
        } else if (!strncasecmp(h, "If-Modified-Since:", strlen("If-Modified-Since:"))) {
            /* This is for debug purposes only, to avoid gzipped stuff */
            headers->lines[t] = NULL;
            continue;
        } else if (!strncasecmp(h, "If-None-Match:", strlen("If-None-Match:"))) {
            /* This is for debug purposes only, to avoid gzipped stuff */
            headers->lines[t] = NULL;
            continue;
        }
#endif

    }

    if ((headers->resp_code == 200 && headers->proto == HTTP_11 && !headers->must_close && !headers->is_chunked && (headers->content_len < 0)
        )
        ) {
        /* Upstream is buggy. */
        nn_log(NN_LOG_NOTICE, "Upstream bug detected on %d response", headers->resp_code);
        headers->must_close = 1;
    }

    /* Restore the default value of content len */
    if (headers->content_len == -1) {
#if DEBUG_CONNECTION >= 5
        nn_log(NN_LOG_DEBUG, "Restoring client content len to zero");
#endif
        headers->content_len = 0;
    }

    return APR_STATUS_SUCCESS;
}

/* ************************************************************************* */

static apr_status_t unpack_headers(nproxy_connection_t * conn, nproxy_connection_side_t * side, nproxy_request_headers_t * headers) {
    char *c = NULL;
    char *bufstr = NULL;

    nn_buffer_get_char(side->buffer, &bufstr);

// TODO MAX header len to prevent dos

    if ((c = strstr(bufstr, "\r\n\r\n")) != NULL) {
        size_t hsize;
        char *lines = NULL;
        char *hl = NULL;
        int hlines = 0;

        // We have all the headers of the request
        hsize = c - bufstr + 4;
        lines = apr_pcalloc(conn->pool, hsize + 1);
        if (!lines) {
            nn_log(NN_LOG_ERROR, "Expecting headers but no headers found");
            return APR_STATUS_ERROR;
        }

        if (nn_buffer_read(side->buffer, lines, hsize) != APR_STATUS_SUCCESS) {
            nn_log(NN_LOG_ERROR, "Cannot read headers from buffer.");
            return APR_STATUS_ERROR;
        }

        headers->rawlen = hsize;
        headers->raw = apr_pstrdup(conn->pool, lines);

        // Count the lines in the header
        hl = lines;
        while (hl && hl < lines + hsize) {
            if ((hl = strstr(hl, "\n")) != NULL) {
                hlines++;
                hl++;
            }
        }

        if (!hlines) {
            nn_log(NN_LOG_ERROR, "Header with no lines.");
            return APR_STATUS_ERROR;
        }

        headers->lines = apr_pcalloc(conn->pool, sizeof(char *) * hlines);
        headers->count = hlines;

        if (apr_separate_string(lines, '\n', headers->lines, hlines) == 0) {
            nn_log(NN_LOG_ERROR, "Cannot separate header lines");
            return APR_STATUS_ERROR;
        } else {
            apr_ssize_t t;
            char *h = NULL;

            for (t = 0; t < hlines; t++) {
                h = headers->lines[t];
                if (!h) {
                    continue;
                }
                chomp(h, strlen(h));
            }
        }
        return APR_STATUS_SUCCESS;
    }

    return APR_STATUS_RETRY;
}

static apr_status_t bridge_data(nproxy_connection_t * conn, side_t sideid, nproxy_request_t * req) {
    apr_size_t lts;
    apr_status_t rv;
    apr_size_t bsize;
    char *bufstr = NULL;
    nproxy_request_headers_t *headers = NULL;
    nproxy_connection_side_t *side = NULL;
    nproxy_connection_side_t *otherside = NULL;

    /* Set the side we're working with */
    if (sideid == SIDE_BROWSER) {
        side = conn->browser;
        otherside = conn->server;
        headers = &req->request_headers;
    } else if (sideid == SIDE_SERVER) {
        side = conn->server;
        otherside = conn->browser;
        headers = &req->response_headers;
    } else {
        return APR_STATUS_ERROR;
    }

    bsize = nn_buffer_get_char(side->buffer, &bufstr);

    if (!bsize) {
        return APR_STATUS_SUCCESS;
    }

    if (req->is_connect) {
        char switch_off = 0;

        if (otherside->sock != NULL) {
            lts = bsize;
            rv = apr_socket_send(otherside->sock, bufstr, &lts);
            if (rv == APR_EOF) {
#if DEBUG_CONNECTION >= 2
                nn_log(NN_LOG_DEBUG, "Bridged connection terminated");
#endif
                switch_off = 1;
            } else if (rv != APR_STATUS_SUCCESS) {
                nn_log(NN_LOG_ERROR, "Error on bridged connection");
                switch_off = 1;
            } else if (lts != bsize) {
                nn_log(NN_LOG_ERROR, "Partially sent buffer content on 'connect' connection (%zd over %zu bytes)", lts, bsize);
                switch_off = 1;
                (sideid == SIDE_BROWSER) ? (req->bytes_out += lts) : (req->bytes_in += lts);
                (sideid == SIDE_BROWSER) ? (conn->bytes_out += lts) : (conn->bytes_in += lts);
            } else {
#if DEBUG_CONNECTION >= 3
                nn_log(NN_LOG_DEBUG, "Bridged %zu bytes", lts);
#endif
                (sideid == SIDE_BROWSER) ? (req->bytes_out += lts) : (req->bytes_in += lts);
                (sideid == SIDE_BROWSER) ? (conn->bytes_out += lts) : (conn->bytes_in += lts);
            }

            /* This is really needed only for CONNECT requests and their shaping */
            headers->content_sent += lts;

        } else {
            switch_off = 1;
        }

        nn_buffer_reset(side->buffer);

        if (switch_off) {
            conn->terminated = 1;
        }
    } else if (!headers->is_chunked) {
        apr_size_t len_to_send;

        if (headers->content_len) {
            len_to_send = headers->content_len - headers->content_sent;
            if (len_to_send > bsize) {
                len_to_send = bsize;
            }
        } else {
            len_to_send = bsize;
        }

        if (len_to_send && len_to_send == bsize) {
            lts = bsize;
            rv = apr_socket_send(otherside->sock, bufstr, &lts);
            if (lts != bsize) {
                nn_log(NN_LOG_ERROR, "Partially sent buffer content on non chunked connection (%zd over %zu bytes)", lts, bsize);
                conn->terminated = 1;
                return APR_STATUS_ERROR;
            }
            (sideid == SIDE_BROWSER) ? (req->bytes_out += lts) : (req->bytes_in += lts);
            (sideid == SIDE_BROWSER) ? (conn->bytes_out += lts) : (conn->bytes_in += lts);

            headers->content_sent += lts;
#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "Sending %zu bytes. Len: %zu/%zu [bsize: %zu]", lts, headers->content_sent, headers->content_len, bsize);
#endif
            nn_buffer_reset(side->buffer);
        } else if (len_to_send > 0) {
            lts = len_to_send;
            rv = apr_socket_send(otherside->sock, bufstr, &lts);
            if (lts != len_to_send) {
                nn_log(NN_LOG_ERROR, "Partially sent buffer content on non chunked connection (%zd over %zu bytes)", lts, bsize);
                conn->terminated = 1;
                return APR_STATUS_ERROR;
            }
            (sideid == SIDE_BROWSER) ? (req->bytes_out += lts) : (req->bytes_in += lts);
            (sideid == SIDE_BROWSER) ? (conn->bytes_out += lts) : (conn->bytes_in += lts);

            headers->content_sent += lts;
#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "Sending %zu bytes. Len: %zu/%zu {bsize: %zu}", lts, headers->content_sent, headers->content_len, bsize);
#endif
            nn_buffer_read(side->buffer, NULL, lts);
        }

        if (headers->content_len && ((headers->content_sent - headers->content_len) == 0)) {
#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "Document is over (not chunked)");
#endif
            if (sideid == SIDE_SERVER) {
                request_purge(conn);
            } else {
                conn->requests->request_headers.document_completed = 1;
            }
        } else if (!headers->content_len && !len_to_send) {
            request_purge(conn);
        }

    } else if (headers->is_chunked) {
        // Chunked payload
        int action = 0;
        size_t len;
        size_t remaining;

        while (!action && (bsize = nn_buffer_get_char(side->buffer, &bufstr))
            ) {
            if (!headers->chunk_len) {
                headers->chunk_sent = 0;
                headers->chunk_len = extract_chunk_size(side->buffer, &action);
#if DEBUG_CONNECTION >= 5
                nn_log(NN_LOG_DEBUG, "New Chunk length: %zu (buffer is %zu - action %d)", headers->chunk_len, bsize, action);
#endif
                if (action == -1) {
                    /* Some errors occurred */
                    return APR_STATUS_ERROR;
                }
                if (action == -2) {
                    /* Retry with a larger buffer */
#if DEBUG_CONNECTION >= 5
                    nn_log(NN_LOG_DEBUG, "Waiting for a larger buffer: %s", bufstr);
#endif
                    return APR_STATUS_NEXT;
                }
            }
            // the max we can send is our buffer size
            len = (bsize < conn->read_buffer_size) ? bsize : conn->read_buffer_size;
            // if we must send less data to finish up the chunk, reduce len accordingly
            remaining = headers->chunk_len - headers->chunk_sent;
            len = (remaining < len) ? remaining : len;

#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "Sending Len: %zu  bsize: %zu", len, bsize);
#endif

            if (len > 0) {
                if (nn_buffer_read(side->buffer, conn->read_buffer, len) != APR_STATUS_SUCCESS) {
                    nn_log(NN_LOG_ERROR, "Cannot read buffer for chunked content");
                    return APR_STATUS_ERROR;
                }

                lts = len;
                rv = apr_socket_send(otherside->sock, bufstr, &lts);
                if (rv == APR_STATUS_SUCCESS || rv == APR_EOF) {
                    headers->chunk_sent += lts;
                    (sideid == SIDE_BROWSER) ? (req->bytes_out += lts) : (req->bytes_in += lts);
                    (sideid == SIDE_BROWSER) ? (conn->bytes_out += lts) : (conn->bytes_in += lts);
                } else {
                    nn_log(NN_LOG_ERROR, "Cannot send chunked content (%zd over %zu)", lts, len);
                    return APR_STATUS_ERROR;
                }
#if DEBUG_CONNECTION >= 5
                nn_log(NN_LOG_DEBUG, "Chunk sent: %zu (%zu/%zu)", lts, headers->chunk_sent, headers->chunk_len);
#endif
            } else {
                // Let's see if there are some leftovers in the trailer
                int loops = 0;

                while (!action) {
                    loops++;
                    assert(loops < 100);

                    bsize = nn_buffer_get_char(side->buffer, &bufstr);
                    if (!bsize) {
                        action = 1;
                    }

                    if (bsize && bsize < 2) {   // Basically, 1
                        nn_log(NN_LOG_ERROR, "UH ? trailers are too short.");
                        return APR_STATUS_ERROR;
                    }

                    if (!strncmp(bufstr, "\r\n", 2)) {
                        char bb[3];

                        if (nn_buffer_read(side->buffer, bb, 2) != APR_STATUS_SUCCESS) {
                            nn_log(NN_LOG_ERROR, "Cannot read buffer removing trailers from chunked content");
                            return APR_STATUS_ERROR;
                        }

                        lts = 2;
                        rv = apr_socket_send(otherside->sock, bb, &lts);
                        if (rv == APR_STATUS_SUCCESS) {
                            (sideid == SIDE_BROWSER) ? (req->bytes_out += lts) : (req->bytes_in += lts);
                            (sideid == SIDE_BROWSER) ? (conn->bytes_out += lts) : (conn->bytes_in += lts);
                        }
                    } else {
                        action = 1;
                    }
                }
            }

            if (action > 0) {
#if DEBUG_CONNECTION >= 5
                nn_log(NN_LOG_DEBUG, "Document is over (chunked)");
#endif
                if (headers->must_close) {
                    if (sideid == SIDE_SERVER) {
#if DEBUG_CONNECTION >= 5
                        nn_log(NN_LOG_DEBUG, "Document is really over so disconnect from the server");
#endif
                        server_force_disconnect(conn);
                    }
                }
                if (sideid == SIDE_SERVER) {
                    request_purge(conn);
                }
                return APR_STATUS_SUCCESS;
            }

            if (headers->chunk_sent == headers->chunk_len) {
                headers->chunk_sent = 0;
                headers->chunk_len = 0;
#if DEBUG_CONNECTION >= 5
                nn_log(NN_LOG_DEBUG, "Passing to next chunk");
#endif
            }
        }                       /* While loop */
    } else {
        /* Should never get here */
        return APR_STATUS_ERROR;
    }

    return APR_STATUS_SUCCESS;
}

static apr_status_t buffer_parse(nproxy_connection_t * conn, side_t sideid) {
    apr_size_t bsize;
    char *bufstr = NULL;
    apr_status_t status;
    nproxy_connection_side_t *side;
    nproxy_connection_side_t *otherside;

    (void) side;
    (void) otherside;

    /* Set the side we're working with */
    if (sideid == SIDE_BROWSER) {
        side = conn->browser;
        otherside = conn->server;
    } else if (sideid == SIDE_SERVER) {
        side = conn->server;
        otherside = conn->browser;
    } else {
        /* Should never get here */
        return APR_STATUS_ERROR;
    }

 restart:
    bsize = nn_buffer_get_char(side->buffer, &bufstr);
#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_DEBUG, "Parse %s buffer: len %zu bytes", (sideid == SIDE_BROWSER) ? "browser" : "server", bsize);
#endif

    /* If the buffer is empty, then return */
    if (!bsize) {
        /* It's worth resetting it not to make the pool grow too much */
        nn_buffer_reset(side->buffer);
        return APR_STATUS_SUCCESS;
    }

    /* If we don't have a request and we ore ont he browser side, then create it */
    if (!conn->requests) {
        nproxy_request_t *r;

        if (sideid == SIDE_BROWSER) {
            r = request_create(conn);
            if (!r) {
                return APR_STATUS_ERROR;
            }
            conn->requests = conn->requests_tail = r;
        } else {
            return APR_STATUS_ERROR;
        }
    }

    /* BROWSER SIDE */
    if (sideid == SIDE_BROWSER) {
        nproxy_request_t *r;

        r = conn->requests_tail;

        if (r->request_headers.document_completed && r->request_headers.rawlen) {
            /* 
               We already have a completed request, so create a new one and
               push it in the queue
             */
            r = request_create(conn);
            if (!r) {
                return APR_STATUS_ERROR;
            }
            r->next = NULL;
            conn->requests_tail->next = r;
            conn->requests_tail = r;

            r = conn->requests_tail;
        }
        /*
           Grab the headers from the buffer if we have enough data.
         */
        if (!r->request_headers.rawlen) {
            int code;
            char *pah = NULL;

            status = unpack_headers(conn, side, &r->request_headers);
            if (status == APR_STATUS_RETRY) {
                return APR_STATUS_SUCCESS;
            }
            if (status == APR_STATUS_ERROR) {
                return APR_STATUS_ERROR;
            }

            /* Parse the client side headers */
            status = parse_client_headers(conn, r);
            if (status != APR_STATUS_SUCCESS) {
                nn_log(NN_LOG_ERROR, "error parsing %s headers", (sideid == SIDE_BROWSER) ? "request" : "response");
//TODO !
//      http_send_custom_response(conn, HTTP_INTERNAL_SERVER_ERROR);
//      nproxy_connection_set_variable(conn, "custom_detail", "Cannot parse client headers");
//      request_purge(conn);
                return status;
            }

            if (conn->auth_user) {
            } else {
                pah = find_header(&r->request_headers, "Proxy-Authorization");
                if (pah) {
                    code = basic_challenge_auth(conn, pah);
                    if (code && (code != 407)) {
                        http_send_custom_response(conn, code);
                        request_purge(conn);
                        on_client_disconnect(conn);
                        return APR_STATUS_SUCCESS;
                    }
                }
            }

            code = lua_call_request(conn);
            if (code && (code != 407)) {
                http_send_custom_response(conn, code);
                request_purge(conn);
                return APR_STATUS_SUCCESS;
            }

            if (conn->requests->require_auth && conn->requests->auth_realm) {
                char *h = NULL;
                h = apr_psprintf(conn->pool, "Proxy-Authenticate: Basic realm=\"%s\"", conn->requests->auth_realm);
                nproxy_connection_set_variable(conn, "additional_headers", h);
                http_send_custom_response(conn, HTTP_PROXY_AUTH_REQ);
                request_purge(conn);
                on_client_disconnect(conn);
                return APR_STATUS_SUCCESS;
            }
        }

    }

    /* SERVER SIDE */
    if (sideid == SIDE_SERVER) {
        nproxy_request_t *r;
        /*
           // Trick to skip the headers which are not expected with HTTP before /1.0.
           if ( side->proto == HTTP_09 ) {
           side->headers = apr_pcalloc(conn->pool, sizeof(char *) * 2 );
           side->must_close = 1;
           otherside->must_close = 1;
           }
         */
        if (!conn->requests->request_headers.count) {
            /* What are we replying to ? */
            assert(0);
            on_client_disconnect(conn);
            return APR_STATUS_ERROR;
        }

        r = conn->requests;

        if (!r->response_headers.headers_sent) {
            int code;
            /*
               Grab the headers from the buffer if we have enough data.
             */
            status = unpack_headers(conn, side, &r->response_headers);
            if (status == APR_STATUS_RETRY) {
                return APR_STATUS_SUCCESS;
            }
            if (status == APR_STATUS_ERROR) {
//TODO !
//      http_send_custom_response(conn, HTTP_INTERNAL_SERVER_ERROR);
//      nproxy_connection_set_variable(conn, "custom_detail", "Cannot unpack server headers");
//      request_purge(conn);
                return APR_STATUS_ERROR;
            }

            status = parse_server_headers(conn, r);
            if (status != APR_STATUS_SUCCESS) {
                nn_log(NN_LOG_ERROR, "error parsing %s headers", (sideid == SIDE_BROWSER) ? "request" : "response");
//TODO !
//      http_send_custom_response(conn, HTTP_INTERNAL_SERVER_ERROR);
//      nproxy_connection_set_variable(conn, "custom_detail", "Cannot parse client headers");
//      request_purge(conn);
                return status;
            }

            if ((code = lua_call_response(conn))) {
                http_send_custom_response(conn, code);
                request_purge(conn);
                server_force_disconnect(conn);
                return APR_STATUS_SUCCESS;
            }
        }
    }

    /*
       Bridge data
     */
    if (sideid == SIDE_BROWSER) {
        nproxy_request_t *r;
        r = conn->requests_tail;

        /*
           We are already connected. Check if it is OK.
         */
        if (conn->server->sock) {
            if (!conn->server->server.host || ((r->force_host == NULL)
                                               && (conn->server->server.port != r->port || strncmp(conn->server->server.host, r->host, strlen(r->host) + 1)
                                               )
                )
                || ((r->force_host != NULL)
                    && (conn->server->server.port != r->force_port || strncmp(conn->server->server.host, r->force_host, strlen(r->force_host) + 1)
                    )
                )
                ) {
#if DEBUG_CONNECTION >= 1
                nn_log(NN_LOG_DEBUG, "Switching connection: remote side has changed.");
                nn_log(NN_LOG_DEBUG, "Switching connection: %s %s", conn->server->server.host, r->host);
                nn_log(NN_LOG_DEBUG, "Switching connection: forced to %s", r->force_host);
#endif
                server_force_disconnect(conn);
            }
        }

        /*
           If we are (no more) connected, reconnect
         */
        if (conn->server->sock == NULL) {
            apr_socket_t *sock;
            char *bind;
            char *host;
            int port;

            if (r->force_host) {
                host = r->force_host;
                port = r->force_port;
                if (!port) {
                    port = 80;
                }
            } else {
                host = r->host;
                port = r->port;
            }

            if (r->force_bind) {
                bind = r->force_bind;
            } else {
                bind = conn->profile->bind_address;
            }

            if ((sock = prepare_outgoing_socket(conn, host, port, bind, conn->ip_v_pref)) == NULL) {
                nn_log(NN_LOG_ERROR, "Cannot connect to %s:%d", host, port);
//TODO timeout or other error, try to figure out what's happened!
                nproxy_connection_set_variable(conn, "custom_detail", "Cannot connect to remote host.");
                http_send_custom_response(conn, HTTP_INTERNAL_SERVER_ERROR);
                request_purge(conn);
                conn->terminated = 1;
                return APR_STATUS_ERROR;
            }

            nn_buffer_reset(conn->server->buffer);

#if DEBUG_CONNECTION >= 1
            nn_log(NN_LOG_DEBUG, "Connection OK to remote party");
#endif
            conn->server->server.host = host;
            conn->server->server.port = port;
            conn->server->sock = sock;

            assert(conn->pollset);

            conn->server->pfd.p = conn->pool;
            conn->server->pfd.desc_type = APR_POLL_SOCKET;
            conn->server->pfd.reqevents = APR_POLLIN;
            conn->server->pfd.rtnevents = 0;
            conn->server->pfd.desc.f = NULL;
            conn->server->pfd.desc.s = sock;
            conn->server->pfd.client_data = NULL;

            apr_pollset_add(conn->pollset, &conn->server->pfd);
        }

        /* Send our data to the remote */
        if (!r->request_headers.headers_sent) {

            if (r->is_connect) {
                apr_size_t ret;
                apr_status_t rv;

                ret = strlen(CONNECT_RESPONSE);
                rv = apr_socket_send(side->sock, CONNECT_RESPONSE, &ret);
                if (rv != APR_STATUS_SUCCESS && rv != APR_EOF) {
                    return APR_STATUS_ERROR;
                }
                r->bytes_in += ret;
                conn->bytes_in += ret;

                ret = strlen(PROXY_AGENT);
                ret = apr_socket_send(side->sock, PROXY_AGENT, &ret);
                if (rv != APR_STATUS_SUCCESS && rv != APR_EOF) {
                    return APR_STATUS_ERROR;    /* Returning an error logs it... this way, at the next read, we should close gracefully  */
                }
                r->bytes_in += ret;
                conn->bytes_in += ret;

                ret = 2;
                ret = apr_socket_send(side->sock, "\r\n", &ret);
                if (rv != APR_STATUS_SUCCESS && rv != APR_EOF) {
                    return APR_STATUS_ERROR;
                }
                r->bytes_in += ret;
                conn->bytes_in += ret;

                r->response_headers.headers_sent = 1;
                r->response_headers.resp_code = 200;
                r->started = apr_time_now();
            } else {
                if (request_send_request_headers(conn, r) != APR_STATUS_SUCCESS) {
                    nn_log(NN_LOG_ERROR, "Cannot send client headers");
                    return APR_STATUS_ERROR;
                }
            }

            r->request_headers.headers_sent = 1;
            r->bytes_out += r->request_headers.rawlen;
            conn->bytes_out += r->request_headers.rawlen;

            if (((r->request_headers.resp_code >= 100 && r->request_headers.resp_code <= 199) ||
                 r->request_headers.resp_code == 204 || r->request_headers.resp_code == 205 || r->request_headers.resp_code == 304)
                ) {
                r->request_headers.document_completed = 1;
#if DEBUG_CONNECTION >= 3
                nn_log(NN_LOG_DEBUG, "Document is over after headers (X04)");
#endif
            } else if (!r->is_connect && !r->request_headers.is_chunked) {
                if (!r->request_headers.content_len && r->request_headers.proto == HTTP_11 && !r->request_headers.must_close) {
#if DEBUG_CONNECTION >= 3
                    nn_log(NN_LOG_DEBUG, "Document is over after headers (content len: %zu)", r->request_headers.content_len);
#endif
                    r->request_headers.document_completed = 1;
                }
            }

            goto restart;
        } else if (!r->request_headers.document_completed) {
            if (bridge_data(conn, sideid, r) != APR_STATUS_SUCCESS) {
                return APR_STATUS_ERROR;
            }
            goto restart;
        } else {
            assert(0);
        }
    } else if (sideid == SIDE_SERVER) {
        nproxy_request_t *r;
        r = conn->requests;

        /* Send our data to the remote */
        if (!r->response_headers.headers_sent) {
            if (request_send_response_headers(conn, r) != APR_STATUS_SUCCESS) {
                nn_log(NN_LOG_ERROR, "Cannot send server headers");
                return APR_STATUS_ERROR;
            }
            r->bytes_in += r->response_headers.rawlen;
            conn->bytes_in += r->response_headers.rawlen;
            r->response_headers.headers_sent = 1;

            if (((r->response_headers.resp_code >= 100 && r->response_headers.resp_code <= 199) ||
                 r->response_headers.resp_code == 204 || r->response_headers.resp_code == 205 || r->response_headers.resp_code == 304)
                ) {
#if DEBUG_CONNECTION >= 3
                nn_log(NN_LOG_DEBUG, "Document is over after headers (X04)");
#endif
                request_purge(conn);
                goto restart;
            }

            if (!r->response_headers.is_chunked) {
                if (!r->response_headers.content_len && r->response_headers.proto == HTTP_11 && !r->response_headers.must_close) {
#if DEBUG_CONNECTION >= 3
                    nn_log(NN_LOG_DEBUG, "Document is over after headers (content len: %zu)", r->response_headers.content_len);
#endif

                    /* 
                       When we receive a 100 Continue, we are about to receive more headers from the server side
                       so just wipe out what we received.
                     */
                    if (r->response_headers.resp_code == 100) {
                        memset(&r->response_headers, 0, sizeof(nproxy_request_headers_t));
                    } else {
                        request_purge(conn);
                        server_force_disconnect(conn);
                    }
                }
            }

            goto restart;
        } else if (!r->response_headers.document_completed) {
            apr_status_t bs;
            bs = bridge_data(conn, sideid, r);

            if (bs == APR_STATUS_NEXT) {
                /* We need more buffer */
                return APR_STATUS_SUCCESS;
            }

            if (bs != APR_STATUS_SUCCESS) {
                return APR_STATUS_ERROR;
            }
            goto restart;
        } else {
            assert(0);
        }

    } else {
        assert(0);
    }

    bsize = nn_buffer_get_char(side->buffer, &bufstr);
#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_DEBUG, "End Parse buffer: len %zu bytes", bsize);
#endif

    return APR_STATUS_SUCCESS;
}

/* ************************************************************************* */

static void adjust_read_buffer_size(nproxy_connection_t * conn) {
    nproxy_request_t *req = conn->requests;

    /*
       Adjust the read buffer size if "delay pools" are active.
       We don't want to sleep more than 0.5 seconds each read so we limit the buffer size accordingly,
       if it is necessary.
     */
    if (req && req->limit_bps && req->response_headers.content_sent > req->limit_max_size) {
        if (conn->read_buffer_size > req->limit_bps / 2 + 1) {
#if DEBUG_CONNECTION >= 7
            nn_log(NN_LOG_DEBUG, "Limiting read buffer size to %zu (was %zu)", req->limit_bps / 2, conn->read_buffer_size);
#endif
            conn->read_buffer_size = req->limit_bps / 2 + 1;
        }
    }

    if (conn && conn->limit_bps && conn->bytes_in > conn->limit_max_size) {
        if (conn->read_buffer_size > conn->limit_bps / 2 + 1) {
#if DEBUG_CONNECTION >= 7
            nn_log(NN_LOG_DEBUG, "Limiting read buffer size to %zu (was %zu)", conn->limit_bps / 2, conn->read_buffer_size);
#endif
            conn->read_buffer_size = conn->limit_bps / 2 + 1;
        }
    }

    return;
}

static void connection_request_sleep(nproxy_connection_t * conn) {
    nproxy_request_t *req = conn->requests;

    /*
       Sleep baby sleep, Now that the night is over,
       you'll be shaped, like a god...
     */
    if (req && req->limit_bps && req->response_headers.content_sent >= req->limit_max_size) {
        apr_time_t now, nt;
        size_t shaped_len;
        double elapsed;
        double expected;

        if (req->limit_started == 0) {
            req->limit_started = apr_time_now();
#if DEBUG_CONNECTION >= 2
            nn_log(NN_LOG_DEBUG, "Enabling throttling (%zu - %zu)", req->limit_max_size, req->limit_bps);
#endif
        } else {
            double msleep_in = 0.0;
            double msleep_out = 0.0;

            now = apr_time_now();
            nt = req->limit_started;

            shaped_len = req->request_headers.content_sent - req->limit_max_size;
            elapsed = (double) apr_time_as_msec(now - nt) / 1000;
            expected = (double) shaped_len / req->limit_bps;

#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "LIO REQ: %d  IN: %d  LEN: %zu   ELAPSED: %.2f  EXPEC: %.2f ", (int) conn->last_io, conn->bytes_in, shaped_len, elapsed, expected);
#endif
            if (elapsed < expected) {
                msleep_out = expected - elapsed;
                if (msleep_out > 0.5) {
                    msleep_out = 0.5;
                }

                if (msleep_out <= 0) {
                    msleep_out = 0.02;
                }
            }

            shaped_len = req->response_headers.content_sent - req->limit_max_size;
            elapsed = (double) apr_time_as_msec(now - nt) / 1000;
            expected = (double) shaped_len / req->limit_bps;

#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "LIO RES: %d  IN: %d  LEN: %zu   ELAPSED: %.2f  EXPEC: %.2f ", (int) conn->last_io, conn->bytes_in, shaped_len, elapsed, expected);
#endif
            if (elapsed < expected) {
                msleep_in = expected - elapsed;
                if (msleep_in > 0.5) {
                    msleep_in = 0.5;
                }

                if (msleep_in <= 0) {
                    msleep_in = 0.02;
                }
            }
#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "Request - Sleep for REQUEST: %.2f    Sleep for RESPONSE: %.2f", msleep_in, msleep_out);
#endif
            if (msleep_in > msleep_out) {
                apr_sleep((apr_interval_time_t) (msleep_in * APR_USEC_PER_SEC));
            } else {
                apr_sleep((apr_interval_time_t) (msleep_out * APR_USEC_PER_SEC));
            }
        }
    }

    if (conn && conn->limit_bps && conn->bytes_in >= conn->limit_max_size) {
        apr_time_t now, then;
        size_t shaped_len;
        double elapsed;
        double expected;

        if (conn->limit_started == 0) {
            conn->limit_started = apr_time_now();
#if DEBUG_CONNECTION >= 2
            nn_log(NN_LOG_DEBUG, "Enabling throttling (%zu - %zu)", conn->limit_max_size, conn->limit_bps);
#endif
        } else {
            double msleep_in = 0.0;
            double msleep_out = 0.0;

            now = apr_time_now();
            then = conn->limit_started;

            shaped_len = conn->bytes_out - conn->limit_max_size;
            elapsed = (double) apr_time_as_msec(now - then) / 1000;
            expected = (double) shaped_len / conn->limit_bps;

#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "LIO REQ: %d  IN: %d  LEN: %zu   ELAPSED: %.2f  EXPEC: %.2f ", (int) conn->last_io, conn->bytes_out, shaped_len, elapsed, expected);
#endif
            if (elapsed < expected) {
                msleep_out = expected - elapsed;
                if (msleep_out > .5) {
                    msleep_out = .5;
                }

                if (msleep_out <= 0) {
                    msleep_out = 0.02;
                }
            }

            shaped_len = conn->bytes_in - conn->limit_max_size;
            elapsed = (double) apr_time_as_msec(now - then) / 1000;
            expected = (double) shaped_len / conn->limit_bps;

#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "LIO RES: %d  IN: %d  LEN: %zu   ELAPSED: %.2f  EXPEC: %.2f ", (int) conn->last_io, conn->bytes_in, shaped_len, elapsed, expected);
#endif
            if (elapsed < expected) {
                msleep_in = expected - elapsed;
                if (msleep_in > .5) {
                    msleep_in = .5;
                }

                if (msleep_in <= 0) {
                    msleep_in = 0.02;
                }
            }
#if DEBUG_CONNECTION >= 5
            nn_log(NN_LOG_DEBUG, "Connection - Sleep for REQUEST: %.2f    Sleep for RESPONSE: %.2f", msleep_in, msleep_out);
#endif

            if (msleep_in > msleep_out) {
                apr_sleep((apr_interval_time_t) (msleep_in * APR_USEC_PER_SEC));
            } else {
                apr_sleep((apr_interval_time_t) (msleep_out * APR_USEC_PER_SEC));
            }

        }
    }

    return;
}

static apr_status_t server_force_disconnect(nproxy_connection_t * conn) {
    nproxy_connection_side_t *side;

    assert(conn);
    side = conn->server;

#if DEBUG_CONNECTION >= 1
    nn_log(NN_LOG_DEBUG, "On server Disconnect");
#endif

    nn_buffer_reset(side->buffer);

    if (side->sock == NULL) {
        return APR_STATUS_SUCCESS;
    }

    if (side->sock) {
        apr_pollset_remove(conn->pollset, &side->pfd);
        while (apr_socket_close(side->sock) != APR_SUCCESS) {
#if DEBUG_CONNECTION >= 1
            nn_log(NN_LOG_DEBUG, "Closing server side socket");
#endif
            apr_sleep(10000);
        }
        side->sock = NULL;
    }

    if (side->server.is_connect) {
        conn->terminated = 1;
    }

    side->server.is_connect = 0;
    side->server.host = NULL;
    side->server.port = 0;

    if (conn->requests && conn->requests->response_headers.must_close
/*
	conn->requests->response_headers.proto == HTTP_10
	||
	conn->requests->response_headers.document_completed
*/
        ) {
        conn->terminated = 1;
    }

    return APR_STATUS_SUCCESS;
}

static void on_server_disconnect(nproxy_connection_t * conn) {

    assert(conn);

    server_force_disconnect(conn);

    //There is a problem here. When switching connection, this is triggered...
    if (conn->requests && conn->requests->request_headers.count && (!conn->requests->response_headers.count || !conn->requests->response_headers.document_completed)
        ) {
        nproxy_connection_set_variable(conn, "custom_detail", "Remote server closed our connection.");
        http_send_custom_response(conn, HTTP_SERVICE_UNAVALAIBLE);
    } else {
        conn->terminated = 1;
    }
    conn->terminated = 1;

    return;
}

static void on_server_read(nproxy_connection_t * conn) {
    apr_size_t tot;
    apr_status_t rv;
    nproxy_connection_side_t *side;

    side = conn->server;

    if (conn->terminated) {
        return;
    }

    adjust_read_buffer_size(conn);
    connection_request_sleep(conn);

    /*
       Read what we can ...
     */
    conn->last_io = apr_time_now();

    tot = conn->read_buffer_size - 1;
    rv = apr_socket_recv(side->sock, conn->read_buffer, &tot);
    if (rv != APR_STATUS_SUCCESS && rv != APR_EOF) {
        /* Error */
#if DEBUG_CONNECTION >= 1
        nn_log(NN_LOG_DEBUG, "Error on server socket");
#endif
        server_force_disconnect(conn);
        return;
    }

    conn->read_buffer[tot] = '\0';

    if (rv == APR_EOF) {
        /* Disconnected */
#if DEBUG_CONNECTION >= 1
        nn_log(NN_LOG_DEBUG, "Disconnect on server socket");
#endif
        on_server_disconnect(conn);
        return;
    }

    if (nn_buffer_append(side->buffer, conn->read_buffer, tot) != APR_STATUS_SUCCESS) {
        on_server_disconnect(conn);
        return;
    }

    if (conn->requests && conn->requests->max_in) {
        if (conn->requests->bytes_in + tot > conn->requests->max_in) {
            nn_log(NN_LOG_NOTICE, "Traffic limit reached (inbound)");
            on_client_disconnect(conn);
            return;
        }
    }

    if (buffer_parse(conn, SIDE_SERVER) != APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_DEBUG, "Error parsing server buffer");
        on_server_disconnect(conn);
    }

    return;
}

/* ************************************************************************* */

static void client_timer_inactivity_check(nproxy_connection_t * conn) {
    apr_time_t now;

    if (conn->terminated) {
        return;
    }

    now = apr_time_now();

    if (conn->last_io && conn->inactivity_timeout) {
        apr_int32_t diff = (apr_int32_t) apr_time_as_msec(now - conn->last_io);
        if (diff >= conn->inactivity_timeout * 1000) {
#if DEBUG_CONNECTION >= 2
            nn_log(NN_LOG_DEBUG, "IO Inactivity timeout, disconnecting client");
#endif
            if (conn->requests && !conn->requests->response_headers.count && !conn->requests->is_connect) {
                nproxy_connection_set_variable(conn, "custom_detail", "Timeout while connecting to remote server");
                nproxy_connection_set_variable(conn, "additional_headers", "Retry-After: 10\r\n");
                http_send_custom_response(conn, HTTP_SERVICE_UNAVALAIBLE);
            }
            conn->terminated = 1;
            return;
        }
    }
#if DEBUG_CONNECTION >= 6
    nn_log(NN_LOG_DEBUG, "Client Inactivity Timer check passed");
#endif

    return;
}

static void client_timer_maxduration_check(nproxy_connection_t * conn) {
    apr_time_t now;

    if (conn->terminated) {
        return;
    }

    now = apr_time_now();

    if (conn->started && conn->max_duration) {
        apr_int32_t diff = (apr_int32_t) apr_time_as_msec(now - conn->started);
        if (diff >= conn->max_duration * 1000) {
#if DEBUG_CONNECTION >= 2
            nn_log(NN_LOG_DEBUG, "Max connection duration reached, disconnecting");
#endif
            conn->terminated = 1;
            return;
        }
    }
#if DEBUG_CONNECTION >= 6
    nn_log(NN_LOG_DEBUG, "Client Max Duration Timer check passed");
#endif

    return;
}

static void on_client_disconnect(nproxy_connection_t * conn) {
    nproxy_connection_side_t *side;

    assert(conn);
    side = conn->browser;

#if DEBUG_CONNECTION > 5
    nn_log(NN_LOG_DEBUG, "On client Disconnect");
#endif

    conn->terminated = 1;
    nn_buffer_reset(side->buffer);

    if (side->sock) {
        apr_pollset_remove(conn->pollset, &side->pfd);
        while (apr_socket_close(side->sock) != APR_SUCCESS) {
#if DEBUG_CONNECTION >= 1
            nn_log(NN_LOG_DEBUG, "Closing client side socket");
#endif
            apr_sleep(10000);
        };
        side->sock = NULL;
    }

    return;
}

static void on_client_read(nproxy_connection_t * conn) {
    apr_size_t tot;
    apr_status_t rv;
    nproxy_connection_side_t *side;

    side = conn->browser;

    if (conn->terminated) {
        return;
    }

    adjust_read_buffer_size(conn);
    connection_request_sleep(conn);

    /*
       Read from our side of the connection ...
     */
    conn->last_io = apr_time_now();

    tot = conn->read_buffer_size - 1;
    rv = apr_socket_recv(side->sock, conn->read_buffer, &tot);

    if (rv != APR_STATUS_SUCCESS && rv != APR_EOF) {
        /* Error */
#if DEBUG_CONNECTION >= 1
        nn_log(NN_LOG_DEBUG, "Error on client socket");
#endif
        on_client_disconnect(conn);
        return;
    }

    conn->read_buffer[tot] = '\0';

#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_DEBUG, "Client read len: %zu", tot);
    /* nn_log(NN_LOG_DEBUG, "%s", conn->read_buffer); */
#endif

    if (nn_buffer_append(side->buffer, conn->read_buffer, tot) != APR_STATUS_SUCCESS) {
        on_client_disconnect(conn);
        return;
    }

    if (conn->requests && conn->requests->max_out) {
        if (conn->requests->bytes_out + tot > conn->requests->max_out) {
            nn_log(NN_LOG_NOTICE, "Traffic limit reached (outbound)");
            on_client_disconnect(conn);
            return;
        }
    }

    if (buffer_parse(conn, SIDE_BROWSER) != APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_DEBUG, "Error parsing client buffer");
        on_client_disconnect(conn);
    }

    if (rv == APR_EOF) {
#if DEBUG_CONNECTION >= 1
        nn_log(NN_LOG_DEBUG, "Disconnect on client socket");
#endif
        on_client_disconnect(conn);
        return;
    }

    return;
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

static void *APR_THREAD_FUNC conn_thread_run(apr_thread_t * UNUSED(thread), void *data) {
    int code;
    char *ip;
    apr_status_t rv;
    apr_sockaddr_t *sa = NULL;
    nproxy_connection_t *conn;

    assert(data);
    conn = (nproxy_connection_t *) data;

    apr_socket_addr_get(&sa, APR_REMOTE, conn->sock);
    apr_sockaddr_ip_get(&ip, sa);

#if DEBUG_CONNECTION >= 1
    nn_log(NN_LOG_DEBUG, "New connection from IP: %s PORT: %d for profile %s", ip, sa->port, conn->profile->name);
#endif

    if ((code = lua_call_connect(conn, ip))) {
        nn_log(NN_LOG_ERROR, "Error running LUA Connect hook (%d)", code);
    }

    apr_pollset_create(&conn->pollset, DEFAULT_POLLSET_NUM, conn->pool, 0);

    conn->browser->pfd.p = conn->pool;
    conn->browser->pfd.desc_type = APR_POLL_SOCKET;
    conn->browser->pfd.reqevents = APR_POLLIN;
    conn->browser->pfd.rtnevents = 0;
    conn->browser->pfd.desc.f = NULL;
    conn->browser->pfd.desc.s = conn->sock;
    conn->browser->pfd.client_data = NULL;

    apr_pollset_add(conn->pollset, &conn->browser->pfd);

    while (!conn->terminated) {
        int i;
        apr_int32_t num;
        apr_socket_t *s;
        const apr_pollfd_t *ret_pfd;

        rv = apr_pollset_poll(conn->pollset, DEFAULT_POLL_TIMEOUT, &num, &ret_pfd);

        if (rv == APR_SUCCESS) {
            for (i = 0; i < num; i++) {
                s = ret_pfd[i].desc.s;

                if (s == conn->browser->sock) {
                    on_client_read(conn);
                    client_timer_maxduration_check(conn);
                } else if (s == conn->server->sock) {
                    on_server_read(conn);
                } else {
                    assert(0);  // UH ? Should never happen ...
                }
            }
        } else {
            client_timer_inactivity_check(conn);
        }
    }

    request_log(conn);

#if DEBUG_CONNECTION >= 1
    nn_log(NN_LOG_DEBUG, "Terminating connection thread");
#endif

    nproxy_connection_destroy(conn);
    return NULL;
}

apr_status_t nproxy_connection_play(nproxy_connection_t * conn) {
    apr_thread_t *thread;
    apr_threadattr_t *thd_attr;

    assert(conn);

    apr_threadattr_create(&thd_attr, conn->pool);
    apr_threadattr_stacksize_set(thd_attr, DEFAULT_THREAD_STACKSIZE);
    apr_threadattr_detach_set(thd_attr, 1);

    if (apr_thread_create(&thread, thd_attr, conn_thread_run, conn, conn->pool) != APR_STATUS_SUCCESS) {
        apr_sockaddr_t *sa;
        char *ip;

        apr_socket_addr_get(&sa, APR_REMOTE, conn->sock);
        apr_sockaddr_ip_get(&ip, sa);

        nn_log(NN_LOG_ERROR, "Cannot start thread on incoming connection from IP: %s PORT: %d for profile %s", ip, sa->port, conn->profile->name);
        return APR_STATUS_ERROR;
    }

    return APR_STATUS_SUCCESS;
}

nproxy_connection_t *nproxy_connection_create(nproxy_profile_t * profile, apr_socket_t * sock, apr_pool_t * pool) {
    nproxy_connection_t *c = NULL;

    assert(pool);

    c = apr_pcalloc(pool, sizeof(nproxy_connection_t));
    if (c != NULL) {
        c->profile = profile;
        c->sock = sock;
        c->pool = pool;

	if ( profile != NULL ) {
	    c->ip_v_pref = profile->ip_v_pref;
	}

        c->browser = apr_pcalloc(pool, sizeof(nproxy_connection_side_t));
        c->server = apr_pcalloc(pool, sizeof(nproxy_connection_side_t));
        if (!c->browser || !c->server) {
            goto err;
        }

        c->browser->sock = sock;

        c->browser->buffer = nn_buffer_init(c->pool, 0, DEFAULT_BUFFER_SIZE);
        if (c->browser->buffer == NULL) {
            goto err;
        }

        c->server->buffer = nn_buffer_init(c->pool, 0, DEFAULT_BUFFER_SIZE);
        if (c->server->buffer == NULL) {
            goto err;
        }

        c->read_buffer_size = DEFAULT_BUFFER_SIZE;
        c->read_buffer = apr_pcalloc(c->pool, c->read_buffer_size);
        if (c->read_buffer == NULL) {
            nn_log(NN_LOG_ERROR, "Cannot alloc context read buffer (%zu bytes)", c->read_buffer_size);
            goto err;
        }

        c->inactivity_timeout = profile->inactivity_timeout;
        c->max_duration = profile->max_duration;
        c->last_io = apr_time_now();
        c->started = apr_time_now();

        c->limit_max_size = profile->limit_max_size;
        c->limit_bps = profile->limit_bps;
    }

    return c;

 err:
    apr_pool_destroy(pool);

    return NULL;
}

void nproxy_connection_destroy(nproxy_connection_t * conn) {
    apr_pool_t *pool;

    assert(conn);

    lua_connection_stop(conn);

    if (conn->browser->sock) {
#if DEBUG_CONNECTION >= 1
        nn_log(NN_LOG_DEBUG, "Closing server side socket.");
#endif
        if (conn->pollset != NULL) {
            apr_pollset_remove(conn->pollset, &conn->browser->pfd);
        }
        apr_socket_close(conn->browser->sock);
    }
    if (conn->server->sock) {
#if DEBUG_CONNECTION >= 1
        nn_log(NN_LOG_DEBUG, "Closing client side socket.");
#endif
        if (conn->pollset != NULL) {
            apr_pollset_remove(conn->pollset, &conn->server->pfd);
        }
        apr_socket_close(conn->server->sock);
    }

    pool = conn->pool;

#ifdef NP_POOL_DEBUG
    {
        apr_size_t s = apr_pool_num_bytes(pool, 1);
        nn_log(NN_LOG_DEBUG, "Connection pool size is %zu", (size_t) s);
    }
#endif

    apr_pool_destroy(pool);

    return;
}
