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
 * nn_log.c
 *
 */

#include "nn_conf.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "nn_log.h"

static nn_log_func_t *cbf = NULL;
static const char *log_text[] = {
    "VERBOSE ",
    "CRITICAL",
    "ERROR   ",
    "WARNING ",
    "NOTICE  ",
    "DEBUG   ",
    "[Woop..]",
    "[Woop..]",
    "[Woop..]",
    "[Woop..]"
};

APR_DECLARE(const char *) nn_log_level_string(nn_log_types level) {
    return log_text[level];
}

APR_DECLARE(void) nn_log_set_function(nn_log_func_t * func) {
    cbf = func;
    return;
}

APR_DECLARE(void) nn_log(nn_log_types level, const char *fmt, ...) {
    va_list ap;

    if (cbf != NULL) {
        va_start(ap, fmt);
        cbf(level, fmt, ap);
        va_end(ap);
    } else {
        size_t len;
        char buf[1024];

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
        va_end(ap);

        fprintf(stderr, "%s- ", log_text[level]);
        fprintf(stderr, "%s", buf);

        len = strlen(buf);
        if ((len > 0) && buf[len - 1] != '\n') {
            fprintf(stderr, "\n");
        }
    }

    return;
}
