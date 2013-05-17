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
 * nn_log.h
 *
 */

#ifndef NN_LOG_H
#define NN_LOG_H

#include "nn_conf.h"
#include "stdarg.h"

typedef enum {
    NN_LOG_VERBOSE = 0,
    NN_LOG_CRIT = 1,
    NN_LOG_ERROR = 2,
    NN_LOG_WARNING = 3,
    NN_LOG_NOTICE = 4,
    NN_LOG_DEBUG = 5
} nn_log_types;

typedef apr_status_t nn_log_func_t(nn_log_types level, const char *fmt, va_list argp);

APR_DECLARE(void) nn_log_set_function(nn_log_func_t * func);
APR_DECLARE(void) nn_log(nn_log_types level, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
APR_DECLARE(const char *) nn_log_level_string(nn_log_types level);

#endif
