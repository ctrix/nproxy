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
 * nn_utils.h
 *
 */

#include "nn_conf.h"
#include "nn_buffer.h"
#include <apr_pools.h>

APR_DECLARE(apr_status_t) apr_file_exists(apr_pool_t * pool, const char *filename);
APR_DECLARE(apr_status_t) apr_is_file(apr_pool_t * pool, const char *filename);
APR_DECLARE(apr_status_t) apr_is_dir(apr_pool_t * pool, const char *filename);

APR_DECLARE(void *) apr_pool_resize(apr_pool_t * pool, void *old_addr, const unsigned long old_byte_size, const unsigned long new_byte_size);

APR_DECLARE(unsigned int) apr_separate_string(char *buf, char delim, char **array, unsigned int arraylen);

#ifndef HAVE_STRCASESTR
APR_DECLARE(char *) strcasestr(const char *haystack, const char *needle);
#endif
