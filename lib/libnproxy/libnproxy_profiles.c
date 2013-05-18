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
 * libnproxy.c
 *
 */

#include "libnproxy.h"

APR_DECLARE(nproxy_profile_t *) nproxy_profile_create(const char *name) {
    apr_pool_t *pool;
    nproxy_profile_t *p = NULL;

    if (zstr(name)) {
        nn_log(NN_LOG_ERROR, "Cannot create a profile without a name");
        return NULL;
    }

    if (apr_pool_create_core(&pool) != APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot initialize profile memory pool");
        return NULL;
    }

    p = apr_pcalloc(pool, sizeof(nproxy_profile_t));
    if (p != NULL) {
        p->pool = pool;
        p->name = apr_pstrdup(pool, name);

        apr_pollset_create(&p->pollset, DEFAULT_POLLSET_NUM, pool, 0);
#ifdef apr_pollset_method_name
        /*
           Older APR versions may not have this function.
           It's easier to conditionally remove this log line
           without testing for the corect APR version.
         */
        nn_log(NN_LOG_DEBUG, "Profile '%s' using pollset method '%s'", name, apr_pollset_method_name(p->pollset));
#endif
    }

    return p;
}

APR_DECLARE(char *) nproxy_profile_get_name(nproxy_profile_t * profile) {
    return profile->name;
}

APR_DECLARE(char *) nproxy_profile_get_script_dir(nproxy_profile_t * profile) {
    if (profile) {
        return profile->script_dir;
    } else {
        return NULL;
    }
}

APR_DECLARE(apr_status_t) nproxy_profile_set_listen(nproxy_profile_t * profile, const char *address, int port) {
    assert(profile);

    if (zstr(address)) {
        profile->listen_address = NULL;
        return APR_STATUS_SUCCESS;
    }

    profile->listen_address = apr_pstrdup(profile->pool, address);
    profile->listen_port = port;

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_bind(nproxy_profile_t * profile, const char *address) {
    assert(profile);

    if (!address || !strlen(address)) {
        profile->bind_address = NULL;
        return APR_STATUS_SUCCESS;
    }

    profile->bind_address = apr_pstrdup(profile->pool, address);

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_maxclient(nproxy_profile_t * profile, int max) {
    assert(profile);

    if (max < 0) {
        return APR_STATUS_ERROR;
    }

    profile->max_clients = max;

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_inactivity_timeout(nproxy_profile_t * profile, int seconds) {
    assert(profile);

    if (seconds >= 0) {
        profile->inactivity_timeout = seconds;
    }

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_max_duration(nproxy_profile_t * profile, int seconds) {
    assert(profile);

    if (seconds >= 0) {
        profile->max_duration = seconds;
    }

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_shaper(nproxy_profile_t * profile, size_t after, size_t bps) {
    assert(profile);

    profile->limit_max_size = after;
    profile->limit_bps = bps;

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_template_dir(nproxy_profile_t * profile, char *dir) {

    profile->template_dir = NULL;

    if (zstr(dir)) {
        return APR_STATUS_ERROR;
    }

    if (apr_is_dir(profile->pool, dir) == APR_STATUS_SUCCESS) {
        profile->template_dir = apr_pstrdup(profile->pool, dir);
        return APR_STATUS_SUCCESS;
    }

    return APR_STATUS_ERROR;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_script_dir(nproxy_profile_t * profile, char *dir) {
    profile->script_dir = NULL;

    if (!dir || !strlen(dir)) {
        return APR_STATUS_SUCCESS;
    }

    if (apr_is_dir(profile->pool, dir) == APR_STATUS_SUCCESS) {
        profile->script_dir = apr_pstrdup(profile->pool, dir);
        return APR_STATUS_SUCCESS;
    }

    return APR_STATUS_ERROR;
}

APR_DECLARE(apr_status_t) nproxy_profile_set_luascript(nproxy_profile_t * profile, char *path) {
    assert(profile);

    if (zstr(path)) {
        profile->luascript = NULL;
        return APR_STATUS_SUCCESS;
    }

    if (apr_file_has_path(path)) {
        profile->luascript = apr_pstrdup(profile->pool, path);
    } else if (!zstr(profile->script_dir)) {
        int len;
        char *str = NULL;
        len = strlen(profile->script_dir) + 1 + strlen(path) + 1;

        str = apr_pcalloc(profile->pool, len);
        if (str == NULL) {
            return APR_STATUS_ERROR;
        }
        snprintf(str, len, "%s" APR_PATH_SEPARATOR "%s", profile->script_dir, path);
        profile->luascript = str;
    } else {
        return APR_STATUS_ERROR;
    }

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(void) nproxy_profile_destroy(nproxy_profile_t * profile) {
    assert(profile);

    apr_pool_destroy(profile->pool);

    return;
}

/* ************************************************************************* */

APR_DECLARE(apr_status_t) nproxy_profile_start(nproxy_profile_t * profile) {
    apr_status_t rv;
    apr_sockaddr_t *sa;
    apr_socket_t *sock;

    assert(profile);

    if (profile->max_clients <= 0) {
        profile->max_clients = DEFAULT_MAX_CLIENTS;
    }

    rv = apr_sockaddr_info_get(&sa, profile->listen_address, APR_UNSPEC, profile->listen_port, 0, profile->pool);
    assert(rv == 0);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot get sockaddr info for profile %s", profile->name);
        goto err;
    }

    rv = apr_socket_create(&sock, sa->family, SOCK_STREAM, APR_PROTO_TCP, profile->pool);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot create socket for profile %s", profile->name);
        goto err;
    }

    rv = apr_socket_opt_set(sock, APR_SO_REUSEADDR, 1);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot reuse addr on socket for profile %s", profile->name);
        goto err;
    }

    rv = apr_socket_bind(sock, sa);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot bind on socket for profile %s", profile->name);
        goto err;
    }

    rv = apr_socket_listen(sock, 5);
    if (rv != APR_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot listen on socket for profile %s", profile->name);
        goto err;
    }

    /* Delay accepting of new connections until data is available */
    apr_socket_opt_set(sock, APR_TCP_DEFER_ACCEPT, 1);

    nn_log(NN_LOG_DEBUG, "Listening socket on %s:%d", profile->listen_address, profile->listen_port);

    profile->sock = sock;

    return APR_STATUS_SUCCESS;

 err:
    return APR_STATUS_ERROR;
}

APR_DECLARE(char) nproxy_profile_running(nproxy_profile_t * profile) {
    return profile->running;
}

APR_DECLARE(apr_status_t) nproxy_profile_stop(nproxy_profile_t * profile) {
    assert(profile);
    profile->stopped = 1;
    return APR_STATUS_SUCCESS;
}

/* ************************************************************************* */

static void on_client_connect(nproxy_profile_t * profile) {
    apr_pool_t *pool = NULL;
    apr_status_t rv;
    apr_socket_t *new_sock;
    nproxy_connection_t *conn;

#if DEBUG_CONNECTION >= 5
    nn_log(NN_LOG_DEBUG, "Accepting client connection");
#endif

    apr_pool_create_core(&pool);

    if (pool != NULL) {
        rv = apr_socket_accept(&new_sock, profile->sock, pool);

        if (rv == APR_SUCCESS) {
            conn = nproxy_connection_create(profile, new_sock, pool);
            if (conn != NULL) {
                if (nproxy_connection_play(conn) != APR_STATUS_SUCCESS) {
                    nproxy_connection_destroy(conn);
                }
            } else {
                apr_socket_close(new_sock);
                apr_pool_destroy(pool);
            }
        } else {
            apr_pool_destroy(pool);
        }
    }

    return;
}

APR_DECLARE(apr_status_t) nproxy_profile_run(nproxy_profile_t * profile) {
    apr_status_t rv;

    assert(profile);

    profile->stopped = 0;
    profile->running = 1;

    {
        apr_pollfd_t pfd = { profile->pool, APR_POLL_SOCKET, APR_POLLIN, 0, {NULL}
        , NULL
        };
        pfd.desc.s = profile->sock;
        apr_pollset_add(profile->pollset, &pfd);
    }

    while (!profile->stopped) {
        apr_int32_t num;
        const apr_pollfd_t *ret_pfd;

        rv = apr_pollset_poll(profile->pollset, DEFAULT_POLL_TIMEOUT, &num, &ret_pfd);
        if (rv == APR_SUCCESS) {
            int i;
            assert(num > 0);
            for (i = 0; i < num; i++) {
                if (ret_pfd[i].desc.s == profile->sock) {
                    on_client_connect(profile);
                } else {
                    /* This should never happen... */
                    assert(0);
                }
            }
        }
    }

    profile->running = 0;

#ifdef NP_POOL_DEBUG
    {

        apr_size_t s = apr_pool_num_bytes(profile->pool, 1);
        nn_log(NN_LOG_DEBUG, "Profile pool size is %zu", (size_t) s);
    }
#endif

    return APR_STATUS_SUCCESS;
}
