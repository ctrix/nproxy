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
 * libnproxy_headers.c
 *
 */

#include "libnproxy.h"

char *find_header(nproxy_request_headers_t * headers, const char *header) {
    apr_size_t t;
    char *h;

    if (!headers) {
        return NULL;
    }

    for (t = 0; t < headers->count; t++) {
        headers->last_script_access = t;
        h = headers->lines[t];

        if (h && !strncasecmp(h, header, strlen(header))) {
            if (*(h + strlen(header)) == ':') {
                h = h + strlen(header) + 1;
                while (*h == ' ') {
                    h++;
                }
                return h;
            }
        }
    }
    return NULL;
}

char *find_header_next(nproxy_request_headers_t * headers, const char *header) {
    apr_size_t t;
    char *h;

    if (!headers) {
        return NULL;
    }

    for (t = headers->last_script_access + 1; t < headers->count; t++) {
        headers->last_script_access = t;
        h = headers->lines[t];

        if (h && !strncasecmp(h, header, strlen(header))) {
            if (*(h + strlen(header)) == ':') {
                h = h + strlen(header) + 1;
                while (*h == ' ') {
                    h++;
                }
                return h;
            }
        }
    }
    return NULL;
}

void clear_header(nproxy_request_headers_t * headers, const char *header) {
    apr_size_t t;
    char *h;

    assert(headers);

    for (t = 0; t < headers->count; t++) {
        h = headers->lines[t];

        if (h && !strncasecmp(h, header, strlen(header))) {
            if (*(h + strlen(header)) == ':') {
                headers->lines[t] = NULL;
                return;
            }
        }
    }

    return;
}

void clear_header_current(nproxy_request_headers_t * headers) {
    apr_size_t t;
    char *h;

    assert(headers);

    if (headers->last_script_access >= headers->count) {
        return;
    }

    t = headers->last_script_access;
    h = headers->lines[t];

    if (h) {
        headers->lines[t] = NULL;
    }

    return;
}

static int find_empty_header(nproxy_request_headers_t * headers) {
    apr_size_t t;
    ssize_t last_empty;
    char *h;

    last_empty = 0;

    for (t = 0; t < headers->count; t++) {
        h = headers->lines[t];
        if (!h) {
            if (!last_empty) {
                last_empty = t;
            }
        } else {
            last_empty = 0;
        }
    }

    return last_empty;
}

void add_header(nproxy_connection_t * conn, nproxy_request_headers_t * headers, const char *text) {
    ssize_t e;
    char *h;

    assert(headers);

    e = find_empty_header(headers);

    if (!e) {
        size_t inc = 5;
        /* We must enlarge the header list we have */
        void *nh;
        unsigned long old_size, new_size;

        old_size = sizeof(char *) * headers->count;
        new_size = sizeof(char *) * (headers->count + inc);

        /* nh = nn_mpool_resize(conn->pool, headers->lines, old_size, new_size, NULL); */
        nh = apr_pcalloc(conn->pool, new_size);

        if (nh == NULL) {
            nn_log(NN_LOG_ERROR, "Cannot add header. Realloc failed.");
            return;
        }

        memcpy(nh, headers->lines, old_size);

        headers->lines = nh;
        headers->count = headers->count + inc;

        e = find_empty_header(headers);

    }

    if (!e) {
        nn_log(NN_LOG_ERROR, "Cannot add header. No space left.");
        return;
    }

    h = apr_pool_strdup(conn->pool, text);
    if (!h) {
        nn_log(NN_LOG_ERROR, "Cannot add header. No space left to dup.");
    } else {
        headers->lines[e] = h;
    }

    return;
}

void replace_header_current(nproxy_connection_t * conn, nproxy_request_headers_t * headers, const char *text) {
    ssize_t t;
    /* char *h; */

    assert(headers);

    if (headers->last_script_access >= headers->count) {
        return;
    }

    t = headers->last_script_access;
    /* h = headers->lines[t]; */

    if (zstr(text)) {
        headers->lines[t] = NULL;
    } else {
        char *nh;
        nh = apr_pool_strdup(conn->pool, text);
        if (nh) {
            headers->lines[t] = nh;
        }
    }

    return;
}
