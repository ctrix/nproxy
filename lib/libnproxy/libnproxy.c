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

typedef struct nproxy_conf_s nproxy_conf_t;

struct nproxy_conf_s {
    char *servername;
    int initialized;
    int stopped;
    apr_hash_t *profile_hash;
};

static apr_pool_t *main_pool = NULL;
static nproxy_conf_t *config = NULL;

/* ************************************************************************* */

APR_DECLARE(apr_status_t) nproxy_init(const char *servername) {

    apr_initialize();

    if (zstr(servername)) {
        nn_log(NN_LOG_DEBUG, "Servername is not set when initializing nproxy library.");
        return APR_STATUS_ERROR;
    }

    if (apr_pool_create_core(&main_pool) != APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Cannot initialize main memory pool");
        return APR_STATUS_ERROR;
    }

    config = apr_pcalloc(main_pool, sizeof(nproxy_conf_t));
    if (config == NULL) {
        return APR_STATUS_ERROR;
    }

    config->servername = apr_pstrdup(main_pool, servername);
    config->initialized = 1;
    config->stopped = 0;

    config->profile_hash = apr_hash_make(main_pool);

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(void) nproxy_deinit(void) {

    config->initialized = 0;
    config->stopped = 1;

    /* The profiles must be shut down by the user of the library */

#ifdef NP_POOL_DEBUG
    {
        apr_size_t s = apr_pool_num_bytes(main_pool, 1);
        nn_log(NN_LOG_DEBUG, "LibNProxy main pool size is %zu", (size_t) s);
    }
#endif

    apr_pool_destroy(main_pool);

    apr_terminate();

    config = NULL;
    return;
}
