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
 * nn_utils.c
 *
 */

#include "nn_conf.h"
#include "nn_log.h"

#include "assert.h"
#include "apr_file_info.h"

/* Returns APR_SUCCESS if the file exists */
APR_DECLARE(apr_status_t) apr_file_exists(apr_pool_t * pool, const char *filename) {
    apr_finfo_t finfo;
    return apr_stat(&finfo, filename, APR_FINFO_TYPE, pool);
}

APR_DECLARE(apr_status_t) apr_is_file(apr_pool_t * pool, const char *filename) {
    apr_finfo_t finfo;

    if (apr_stat(&finfo, filename, APR_FINFO_TYPE, pool) != APR_INCOMPLETE) {
        if (finfo.filetype == APR_REG) {
            return APR_STATUS_SUCCESS;
        }
    }

    return APR_STATUS_ERROR;
}

APR_DECLARE(apr_status_t) apr_is_dir(apr_pool_t * pool, const char *filename) {
    apr_finfo_t finfo;

    if (apr_stat(&finfo, filename, APR_FINFO_TYPE, pool) != APR_INCOMPLETE) {
        if (finfo.filetype == APR_DIR) {
            return APR_STATUS_SUCCESS;
        }
    }

    return APR_STATUS_ERROR;
}

APR_DECLARE(char *) apr_pool_strdup(apr_pool_t * pool, const char *string) {
    char *ret = NULL;

    if ((pool != NULL) && (string != NULL)) {
        size_t len = strlen(string);

        ret = apr_palloc(pool, len + 1);
        if (ret != NULL) {
            memcpy(ret, string, len + 1);
        }
    }

    return ret;
}

APR_DECLARE(void *) apr_pool_resize(apr_pool_t * pool, void *old_addr, const unsigned long old_byte_size, const unsigned long new_byte_size) {
    void *new;

    /* If the size is shrinking, then ignore ... */
    if (new_byte_size <= old_byte_size) {
        return old_addr;
    }

    new = apr_palloc(pool, new_byte_size);
    if (new != NULL) {
        memcpy(new, old_addr, old_byte_size);
        return new;
    } else {
        return NULL;
    }
}

/* This function kindly inspired from anthm - Freeswitch */
APR_DECLARE(unsigned int) apr_separate_string(char *buf, char delim, char **array, unsigned int arraylen) {
    unsigned int argc;
    char *ptr;
    int quot = 0;
    char qc = '"';
    char *e;
    unsigned int x;

    if (!buf || !array || !arraylen) {
        return 0;
    }

    memset(array, 0, arraylen * sizeof(*array));

    ptr = buf;

    for (argc = 0; *ptr && (argc < arraylen - 1); argc++) {
        array[argc] = ptr;
        for (; *ptr; ptr++) {
            if (*ptr == qc) {
                if (quot) {
                    quot--;
                } else {
                    quot++;
                }
            } else if ((*ptr == delim) && !quot) {
                *ptr++ = '\0';
                break;
            }
        }
    }

    if (*ptr) {
        array[argc++] = ptr;
    }

    /* strip quotes */
    for (x = 0; x < argc; x++) {
        if (*(array[x]) == qc) {
            (array[x])++;
            if ((e = strchr(array[x], qc))) {
                *e = '\0';
            }
        }
    }

    return argc;
}
