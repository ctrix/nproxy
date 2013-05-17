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
 * nn_buffer.h
 *
 */

#ifndef NN_BUFFER_H
#define NN_BUFFER_H

#include "nn_conf.h"
#include <apr_pools.h>

enum nn_buffer_flags_e {
    NN_BUF_FLAG_STRING = (1 << 0),
    NN_BUF_FLAG_NO_PREPEND = (1 << 1),
    NN_BUF_FLAG_DOUBLE = (1 << 2),
};

typedef enum nn_buffer_flags_e nn_buffer_flags_t;

typedef struct nn_buffer_s nn_buffer_t;

APR_DECLARE(nn_buffer_t *) nn_buffer_init(apr_pool_t * pool, apr_uint16_t flags, apr_uint32_t start_size);
APR_DECLARE(void) nn_buffer_grow_size(nn_buffer_t * buf, apr_uint32_t size);
APR_DECLARE(apr_status_t) nn_buffer_reset(nn_buffer_t * buf);
APR_DECLARE(apr_status_t) nn_buffer_autoshrink(nn_buffer_t * buf);
APR_DECLARE(apr_status_t) nn_buffer_enlarge(nn_buffer_t * buf, apr_uint32_t size);

APR_DECLARE(apr_status_t) nn_buffer_printf(nn_buffer_t * buf, const char *format, ...) __attribute__ ((format(printf, 2, 3)));
APR_DECLARE(apr_status_t) nn_buffer_append(nn_buffer_t * buf, const void *src, apr_uint32_t size);
APR_DECLARE(apr_status_t) nn_buffer_appendc(nn_buffer_t * buf, const char c);
APR_DECLARE(apr_status_t) nn_buffer_prepend(nn_buffer_t * buf, const void *src, apr_uint32_t size);

APR_DECLARE(apr_status_t) nn_buffer_read(nn_buffer_t * buf, char *dest, apr_uint32_t size);
APR_DECLARE(apr_status_t) nn_buffer_extract(nn_buffer_t * buf, char *dest, apr_uint32_t size, int sep);

APR_DECLARE(apr_uint32_t) nn_buffer_get_char(nn_buffer_t * buf, char **dest);
APR_DECLARE(apr_uint32_t) nn_buffer_capacity(nn_buffer_t * buf);
APR_DECLARE(apr_uint32_t) nn_buffer_size(nn_buffer_t * buf);
APR_DECLARE(apr_uint32_t) nn_buffer_size_left(nn_buffer_t * buf);
APR_DECLARE(apr_status_t) nn_buffer_rollback(nn_buffer_t * buf);
APR_DECLARE(apr_status_t) nn_buffer_destroy(nn_buffer_t * buf);

APR_DECLARE(apr_status_t) nn_buffer_dump(nn_buffer_t * buf, const char *fname);

#endif
