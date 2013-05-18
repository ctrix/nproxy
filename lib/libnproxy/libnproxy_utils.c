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
 * libnproxy_utils.c
 *
 */

#include "libnproxy.h"

apr_status_t chomp(char *buffer, size_t length) {
    size_t chars;

    if (!length) {
        length = strlen(buffer);
    }

    if (!length) {
        return APR_STATUS_SUCCESS;
    }

    if (buffer == NULL) {
        return APR_STATUS_ERROR;
    }
    if (length < 1) {
        return APR_STATUS_ERROR;
    }

    chars = 0;

    --length;
    while (buffer[length] == '\r' || buffer[length] == '\n') {
        buffer[length] = '\0';
        chars++;

        /* Stop once we get to zero to prevent wrap-around */
        if (length-- == 0) {
            break;
        }
    }

    return APR_STATUS_SUCCESS;
}

apr_socket_t *prepare_outgoing_socket(nproxy_connection_t * conn, const char *host, int port, const char *bind_to) {
    apr_sockaddr_t *sa = NULL;
    apr_socket_t *sock = NULL;
    apr_status_t rv;
    apr_pool_t *pool;

    assert(conn);
    assert(host != NULL);
    assert(port > 0);

    pool = conn->pool;

#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_DEBUG, "Connection to HOST: %s  PORT: %d binding to: %s", host, port, (bind_to) ? bind_to : "default");
#endif

    /* TODO ASD Preference v4/v6 */

    rv = apr_sockaddr_info_get(&sa, host, APR_INET, port, 0, pool);
    //rv = apr_sockaddr_info_get(&sa, host, APR_INET6, port, 0, pool);
    //rv = apr_sockaddr_info_get(&sa, host, APR_UNSPEC, port, 0, pool);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot get sockaddr info");
        return NULL;
    }

    rv = apr_socket_create(&sock, sa->family, SOCK_STREAM, APR_PROTO_TCP, pool);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot create outgoing socket");
        return NULL;
    }

    if (!zstr(bind_to)) {
        apr_sockaddr_t *sb;
// TODO QWE BINDING DO TEST IT
        rv = apr_sockaddr_info_get(&sb, bind_to, APR_UNSPEC, 0, 0, pool);
        rv = apr_socket_bind(sock, sb);
        if (rv != APR_SUCCESS) {
            nn_log(NN_LOG_ERROR, "Cannot bind outgoing connecton to %s", bind_to);
            return NULL;
        }
    }

    rv = apr_socket_connect(sock, sa);
    if (rv != APR_STATUS_SUCCESS) {
        if (bind_to) {
            nn_log(NN_LOG_ERROR, "Could not establish a connection to %s:%d binding to address %s", host, port, bind_to);
        } else {
            nn_log(NN_LOG_ERROR, "Could not establish a connection to %s:%d", host, port);
        }
        return NULL;
    }
#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_ERROR, "Successfull connection to %s:%d", host, port);
#endif

    return sock;
}

/*
    action == 1  doc is over
    action == 0  returned value is OK
    action == -1 some errors occurred
    action == -2 retry please! Not enouth data
*/
size_t extract_chunk_size(nn_buffer_t * buf, int *action) {
    int bsize;
    size_t ret = 0;
    char *bc;
    char *endline;
    char line[64];
    char *linep = line;
    char must_free = 0;
    size_t linelen;
    char trailers = 0;

    bsize = nn_buffer_get_char(buf, &bc);
    if (!bsize) {
        *action = -2;
        return 0;
    }

    endline = strchr(bc, '\n');
    if (!endline) {
        *action = -2;
        return 0;
    }

    linelen = endline - bc + 1;

    if (linelen > sizeof(line) - 1) {
        /* 
           If we really need, we malloc the memory needed
           It's better not to alloc in the pool too many data.
         */
        linep = malloc(linelen);
        if (!linep) {
            *action = -1;
            return 0;
        }
        must_free = 1;
    }

    memset(linep, 0, linelen);
    memcpy(linep, bc, linelen - 1);
    chomp(linep, strlen(line));

    ret = strtol(linep, NULL, 16);
    if (ret == 0) {
        *action = 1;
    }

    if (ret) {
#if DEBUG_CONNECTION >=9
        nn_log(NN_LOG_DEBUG, "Chunk len: %zu - '%s' (%zu)", ret, linep, linelen);
#endif
    } else {
        if (!strncmp(bc + linelen, "\r\n", 2)) {
            trailers = 1;
#if DEBUG_CONNECTION >=9
            nn_log(NN_LOG_DEBUG, "Adding 2 more chars");
#endif
        }
/*
#if DEBUG_CONNECTION >=9
        nn_log(NN_LOG_DEBUG, "Chunk len: %d - 'NONE' (%d) \n%s\n", ret, linelen, linep);
        nn_log(NN_LOG_DEBUG, "**\n'%s'\n**\n", bc);
#endif
*/
    }

    if (ret) {
        ret += 2;
    }
    ret += linelen;

    if (trailers) {
        ret += 2;
    }

    if (must_free) {
        free(linep);
    }

    return ret;
}
