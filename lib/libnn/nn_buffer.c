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
 * nn_buffer.c
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"

#include "nn_conf.h"

#include "nn_log.h"
#include "nn_buffer.h"
#include "nn_utils.h"

#define NN_BUF_DEFAULT_LEN	8192

#define BPTR(buf)  ((buf)->data + (buf)->offset)
#define BEND(buf)  (BPTR(buf) + (buf)->len)
#define BLEN(buf)  ((buf)->len)
#define BCAP(buf)  (buf_forward_capacity (buf))

struct nn_buffer_s {
    apr_pool_t *pool;
    apr_uint16_t flags;
    apr_uint32_t max_grow_size;
    apr_uint32_t capacity;

    apr_uint32_t offset;
    apr_uint32_t len;
    apr_uint8_t *data;
    apr_uint32_t old_offset;
    apr_uint32_t old_len;
};

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

static NN_INLINE apr_uint32_t buf_forward_capacity(nn_buffer_t * buf) {
    int ret = buf->capacity - (buf->offset + buf->len);
    if (ret < 0)
        ret = 0;
    return ret;
}

static NN_INLINE int buf_safe(nn_buffer_t * buf, int len) {
    return len >= 0 && buf->offset + buf->len + len <= buf->capacity;
}

static NN_INLINE apr_uint8_t *buf_write_alloc(nn_buffer_t * buf, apr_uint32_t size) {
    apr_uint8_t *ret;

    if (!buf_safe(buf, size))
        return NULL;

    buf->old_len = buf->len;
    buf->old_offset = buf->offset;

    ret = BPTR(buf) + buf->len;
    buf->len += size;
    return ret;
}

static NN_INLINE apr_uint8_t *buf_read_alloc(nn_buffer_t * buf, apr_uint32_t size) {
    apr_uint8_t *ret;

    if (buf->len < size) {
        return NULL;
    }

    ret = BPTR(buf);
    buf->offset += size;
    buf->len -= size;
    return ret;
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

APR_DECLARE(nn_buffer_t *) nn_buffer_init(apr_pool_t * main_pool, apr_uint16_t flags, apr_uint32_t start_size) {
    apr_pool_t *pool;
    nn_buffer_t *buf;

    assert(main_pool);

    apr_pool_create(&pool, main_pool);
    assert(pool);

    if (start_size == 0) {
        start_size = NN_BUF_DEFAULT_LEN;
    }

    buf = apr_palloc(pool, sizeof(nn_buffer_t));

    if (buf) {
        buf->pool = pool;
        buf->flags = flags;
        buf->capacity = start_size;
        buf->len = 0;
        buf->offset = 0;
        buf->old_len = 0;
        buf->old_offset = 0;
        buf->max_grow_size = start_size;
        buf->data = apr_palloc(pool, buf->capacity);
        if (buf->data) {
            *(buf->data) = '\0';
        } else {
            buf = NULL;
        }
    }

    return buf;
}

APR_DECLARE(void) nn_buffer_grow_size(nn_buffer_t * buf, apr_uint32_t size) {
    assert(buf);
    buf->max_grow_size = size;
}

APR_DECLARE(apr_status_t) nn_buffer_reset(nn_buffer_t * buf) {
    assert(buf);
    buf->len = 0;
    buf->old_len = 0;
    buf->offset = 0;
    buf->old_offset = 0;
    //memset(buf->data, 0, buf->capacity);
    *(buf->data) = '\0';
    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_autoshrink(nn_buffer_t * buf) {
    assert(buf);

    /* 
       if the offset is bigger than (about) 30% of the 
       total capacity, we're wasting space.
       So move the data back to the beginning of the buffer
     */
    if (buf->offset && buf->offset > buf->capacity / 3) {
        buf->old_len = buf->len;
        buf->old_offset = buf->offset;
        memmove(buf->data, buf->data + buf->offset, buf->len + 1);
        buf->offset = 0;
        memset(buf->data + buf->offset + buf->len, 0, 1);
        *(BEND(buf)) = '\0';
    }

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_enlarge(nn_buffer_t * buf, apr_uint32_t size) {
    void *newdata = NULL;

    assert(buf);
    if (size == 0) {
        size = buf->capacity;
    } else if (buf->flags & NN_BUF_FLAG_DOUBLE) {
        size = buf->capacity;
    }

    if ((buf->max_grow_size > 0) && (size > buf->max_grow_size)) {
        size = buf->max_grow_size;
    }

    newdata = apr_pool_resize(buf->pool, buf->data, buf->capacity, buf->capacity + size);

    if (!newdata) {
        return APR_STATUS_ERROR;
    }

    buf->data = newdata;
    buf->capacity += size;

    return APR_STATUS_SUCCESS;
}

apr_status_t nn_buffer_printf(nn_buffer_t * buf, const char *format, ...) {
    char *str;
    va_list arglist;

    va_start(arglist, format);
    str = apr_pvsprintf(buf->pool, format, arglist);
    va_end(arglist);

    if (str == NULL) {
        return APR_STATUS_ERROR;
    }

    return nn_buffer_append(buf, str, strlen(str));
}

APR_DECLARE(apr_status_t) nn_buffer_append(nn_buffer_t * buf, const void *src, apr_uint32_t size) {
    apr_uint8_t *cp = NULL;

    if (!src) {
        return APR_STATUS_ERROR;
    }

    if (!size) {
        size = strlen((char *) src);
    }

    if (size <= 0) {
        return APR_STATUS_SUCCESS;
    }

    cp = buf_write_alloc(buf, size);
    if (cp == NULL) {
        if (nn_buffer_enlarge(buf, size) == APR_STATUS_SUCCESS) {
            return nn_buffer_append(buf, src, size);
        } else {
            nn_log(NN_LOG_WARNING, "Cannot enlarge our buffer size\n");
            return APR_STATUS_ERROR;
        }
    } else {
        memcpy(cp, src, size);
        //memmove (cp, src, size);
        *(BEND(buf)) = 0;
    }

    nn_buffer_autoshrink(buf);

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_appendc(nn_buffer_t * buf, const char c) {
    apr_uint8_t *cp;

    nn_buffer_autoshrink(buf);

    cp = buf_write_alloc(buf, 1);
    if (!cp) {
        /*
           Enlarge by 1024 bytes even if we need only one to
           prevent too many re-enlargements that would waste too
           much memory
         */
        nn_buffer_enlarge(buf, 1024);
        return nn_buffer_appendc(buf, c);
    } else {
        memcpy(cp, &c, 1);
        *(BEND(buf)) = 0;       // TOD This makes our life easier with strings. But is it safe ? (boundary check)
    }

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_prepend(nn_buffer_t * buf, const void *src, apr_uint32_t size) {
    /* Cannot prepend, should move the data or resize our data pool */
    assert(buf);

    /* This buffer is not suitable to prepend any data. */
    if (buf->flags & NN_BUF_FLAG_NO_PREPEND) {
        return APR_STATUS_ERROR;
    }

    if (!src) {
        return APR_STATUS_ERROR;
    }

    if (!size)
        size = strlen(src);

    if (BCAP(buf) >= size) {
        /* We can move our data */
        buf->old_len = buf->len;
        buf->old_offset = size;

        memmove(BPTR(buf) + size, BPTR(buf), BLEN(buf));
        buf->len += size;
        memcpy(BPTR(buf), src, size);
        return APR_STATUS_SUCCESS;
    }

    return APR_STATUS_ERROR;
}

APR_DECLARE(apr_status_t) nn_buffer_read(nn_buffer_t * buf, char *dest, apr_uint32_t size) {
    apr_uint8_t *cp = NULL;
    assert(buf);

    cp = buf_read_alloc(buf, size);
    if (!cp) {
        return APR_STATUS_ERROR;
    } else {
        /* If we don't have any destination, this function simply removes data from the buffer. */
        if (dest) {
            memcpy(dest, cp, size);
        }
    }

    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_extract(nn_buffer_t * buf, char *dest, apr_uint32_t size, int sep) {
    apr_uint32_t rlen;
    apr_uint8_t *c;
    char *bufstr = (char *) BPTR(buf);

    assert(buf);

    /* If we have the separator */
    if ((c = (apr_uint8_t *) strchr(bufstr, sep)) != NULL) {
        /* If the resulting string is short enough to fit into our destination */
        if (c < buf->data + buf->offset + buf->len) {
            rlen = c - (buf->data + buf->offset) + 1;
            memset(dest + size, 0, 1);
            return nn_buffer_read(buf, dest, rlen);
        }
    }
    return APR_STATUS_ERROR;
}

APR_DECLARE(apr_uint32_t) nn_buffer_get_char(nn_buffer_t * buf, char **dest) {
    apr_uint32_t len;

    if (dest) {
        *dest = (char *) BPTR(buf);
    }

    len = BLEN(buf);
    return len;
}

APR_DECLARE(apr_uint32_t) nn_buffer_size(nn_buffer_t * buf) {
    assert(buf);
    return BLEN(buf);
}

APR_DECLARE(apr_uint32_t) nn_buffer_capacity(nn_buffer_t * buf) {
    return buf->capacity;
}

APR_DECLARE(apr_uint32_t) nn_buffer_size_left(nn_buffer_t * buf) {
    assert(buf);
    return BCAP(buf);
}

APR_DECLARE(apr_status_t) nn_buffer_rollback(nn_buffer_t * buf) {
    assert(buf);
    buf->offset = buf->old_offset;
    buf->len = buf->old_len;
    *(BEND(buf)) = 0;           /* windows vsnprintf needs this */
    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_destroy(nn_buffer_t * buf) {
    assert(buf);
    apr_pool_destroy(buf->pool);
    return APR_STATUS_SUCCESS;
}

APR_DECLARE(apr_status_t) nn_buffer_dump(nn_buffer_t * buf, const char *fname) {
    int blen;
    char *tmp;
    FILE *f;

    if (!buf || !fname)
        return APR_STATUS_ERROR;

    blen = nn_buffer_get_char(buf, &tmp);

    f = fopen(fname, "w");
    if (f) {
        size_t wcount;
        wcount = fwrite(tmp, 1, blen, f);
        (void) wcount;
        fclose(f);
        return APR_STATUS_SUCCESS;
    }
    return APR_STATUS_ERROR;
}
