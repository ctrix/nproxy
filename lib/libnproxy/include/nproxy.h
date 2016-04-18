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
 * nproxy.h
 *
 */

#ifndef NPROXY_H
#define NPROXY_H

#include "apr.h"
#include "apr_errno.h"          /* Needed for apr_status_t */

typedef struct nproxy_profile_s nproxy_profile_t;
typedef struct nproxy_profile_stats_s nproxy_profile_stats_t;
typedef struct nproxy_connection_s nproxy_connection_t;

APR_DECLARE(apr_status_t) nproxy_init(const char *servername);
APR_DECLARE(void) nproxy_deinit(void);

APR_DECLARE(nproxy_profile_t *) nproxy_profile_create(const char *name);
APR_DECLARE(char *) nproxy_profile_get_name(nproxy_profile_t * profile);
APR_DECLARE(char *) nproxy_profile_get_script_dir(nproxy_profile_t * profile);

APR_DECLARE(apr_status_t) nproxy_profile_set_listen(nproxy_profile_t * profile, const char *address, int port);
APR_DECLARE(apr_status_t) nproxy_profile_set_bind(nproxy_profile_t * profile, const char *address);
/*
TODO - Not yet implemented ...
APR_DECLARE(apr_status_t) nproxy_profile_set_maxclient(nproxy_profile_t *profile, int max);
*/
APR_DECLARE(apr_status_t) nproxy_profile_set_v6_preference(nproxy_profile_t * profile, int prefer_v6);
APR_DECLARE(apr_status_t) nproxy_profile_set_inactivity_timeout(nproxy_profile_t * profile, int seconds);
APR_DECLARE(apr_status_t) nproxy_profile_set_max_duration(nproxy_profile_t * profile, int seconds);
APR_DECLARE(apr_status_t) nproxy_profile_set_shaper(nproxy_profile_t * profile, size_t after, size_t bps);
APR_DECLARE(apr_status_t) nproxy_profile_set_template_dir(nproxy_profile_t * profile, char *dir);
APR_DECLARE(apr_status_t) nproxy_profile_set_script_dir(nproxy_profile_t * profile, char *dir);
APR_DECLARE(apr_status_t) nproxy_profile_set_luascript(nproxy_profile_t * profile, char *path);

APR_DECLARE(void) nproxy_profile_destroy(nproxy_profile_t * profile);

APR_DECLARE(apr_status_t) nproxy_profile_start(nproxy_profile_t * profile);
APR_DECLARE(apr_status_t) nproxy_profile_run(nproxy_profile_t * profile);
APR_DECLARE(char) nproxy_profile_running(nproxy_profile_t * profile);
APR_DECLARE(apr_status_t) nproxy_profile_stop(nproxy_profile_t * profile);

#endif
