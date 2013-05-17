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
 * nproxyd_log.c
 *
 */

#include "nn_conf.h"

#include "apr_strings.h"
#include "apr_file_io.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>

#include "nn_log.h"
#include "nn_xml.h"
#include "nn_utils.h"

#include "nproxyd.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#define DATEFORMAT "%b %d %H:%M:%S"

struct log_setting_s {
    apr_file_t *eventlog;
    char *log_file;
    unsigned int log_file_level;
    char *syslog_ident;
    unsigned int syslog_level;
    int stdout_loglevel;
    short int always_do_stdout;
};

typedef struct log_setting_s log_setting_t;

static int initialized = 0;
static log_setting_t *globals = NULL;
static apr_pool_t *global_pool = NULL;
static int pool_is_mine = 0;

/*****************************************************************************

 ******************************************************************************/

#define LOG_LINE_SIZE 4096

static apr_status_t nproxy_log_function(nn_log_types level, const char *fmt, va_list ap) {
    int len;
    time_t t;
    struct tm tm, *tm_p;
    char date[32];
    char buf[LOG_LINE_SIZE];

    const char *slevel = nn_log_level_string(level);

    memset(buf, 0, sizeof(buf));

    time(&t);
#if defined(localtime_r)
    localtime_r(&t, &tm);
#else
    /* Not the safest thing but windows doesn't have localtime_r */
    tm_p = localtime(&t);
    memcpy(&tm, tm_p, sizeof(struct tm));
#endif
    strftime(date, sizeof(date), DATEFORMAT, &tm);

    /* Creating the log line */
    len = vsnprintf(buf, sizeof(buf) - 2, fmt, ap);

    /* The line was too long ... */
    if (len > LOG_LINE_SIZE) {
        len = LOG_LINE_SIZE - 2;
        buf[len] = '\0';
    }

    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        len--;
    }

    /*
       stdout
     */
    if (!initialized || (globals && globals->always_do_stdout)) {
        fprintf(stderr, "%s- ", slevel);
        fprintf(stderr, "%s\n", buf);
    }

    /*
       Syslog
     */
#ifdef HAVE_SYSLOG_H
    if (globals->syslog_ident && (level <= globals->syslog_level) && strlen(globals->syslog_ident) > 0) {
        /* Remove the \n at the end of the line. OSX (for example) prints it out in the logs. Awful. */
        syslog(LOG_ERR, "%s %s", slevel, buf);
    }
#endif

    /*
       Log file
     */
    if ((globals->eventlog != NULL) && (level <= globals->log_file_level)) {
        apr_size_t wl;
        buf[len++] = '\n';
        wl = strlen(date);
        apr_file_write(globals->eventlog, date, &wl);
        wl = 1;
        apr_file_write(globals->eventlog, " ", &wl);
        wl = strlen(slevel);
        apr_file_write(globals->eventlog, slevel, &wl);
        wl = 2;
        apr_file_write(globals->eventlog, "- ", &wl);
        wl = len;
        apr_file_write(globals->eventlog, buf, &wl);
    }

    return APR_STATUS_SUCCESS;
}

/*****************************************************************************

 ******************************************************************************/

static apr_status_t log_parse_config(const char *cfname) {
    apr_status_t status = APR_STATUS_ERROR;

    nn_xml_t cfg, xml = NULL, param, settings;

    globals->log_file = NULL;
    globals->syslog_ident = NULL;

    if (!(cfg = nn_xml_parse_file(cfname))) {
        nn_log(NN_LOG_ERROR, "Open of %s failed.", cfname);
        goto done;
    }

    xml = cfg;

    if ((settings = nn_xml_child(xml, "log-settings"))) {
        for (param = nn_xml_child(settings, "param"); param; param = param->next) {

            char *var = (char *) nn_xml_attr_soft(param, "name");
            char *val = (char *) nn_xml_attr_soft(param, "value");

            if (var && !strlen(var))
                continue;
            if (val && !strlen(val))
                continue;

            nn_log(NN_LOG_DEBUG, " > %s = %s \n", var, val);

            if (!strcasecmp(var, "log-file")) {
                if (strlen(val)) {
                    globals->log_file = apr_pstrdup(global_pool, val);
                }
            } else if (!strcasecmp(var, "log-file-level")) {
                globals->log_file_level = atoi(val);
            } else if (!strcasecmp(var, "syslog-ident")) {
                globals->syslog_ident = apr_pstrdup(global_pool, val);
            } else if (!strcasecmp(var, "syslog-level")) {
                globals->syslog_level = atoi(val);
            }
        }
    } else {
        nn_log(NN_LOG_ERROR, "Log settings not found in %s", cfname);
        goto done;
    }

    if (!globals->log_file_level)
        globals->log_file_level = 1;

    if (!globals->syslog_level)
        globals->syslog_level = 1;

    status = APR_STATUS_SUCCESS;

 done:
    nn_xml_free(xml);

    return status;
}

/*****************************************************************************

 ******************************************************************************/

static apr_status_t nproxy_log_fopen(void) {

    /*
       This initialize the file logging facility
     */
    if (!zstr(globals->log_file)) {
        if (APR_STATUS_SUCCESS == apr_file_open(&globals->eventlog, globals->log_file, APR_CREATE | APR_APPEND | APR_WRITE, APR_FPROT_UREAD | APR_FPROT_UWRITE | APR_FPROT_GREAD, global_pool)) {
            nn_log(NN_LOG_DEBUG, "Log file opened and initialized");
        } else {
            nn_log(NN_LOG_ERROR, "Unable to open log file '%s'", globals->log_file);
        }
    }
#ifdef HAVE_SYSLOG_H
    /* This inits the syslog facility */
    if (!zstr(globals->syslog_ident)) {
        openlog(globals->syslog_ident, LOG_PID, LOG_USER);
    }
#endif
    return APR_STATUS_SUCCESS;
}

static apr_status_t nproxy_log_fclose(void) {

    if (globals->eventlog != NULL) {
        apr_file_close(globals->eventlog);
        globals->eventlog = NULL;
    }
#ifdef HAVE_SYSLOG_H
    if (!zstr(globals->syslog_ident)) {
        closelog();
    }
#endif

    return APR_STATUS_SUCCESS;
}

/*****************************************************************************

 ******************************************************************************/

void nproxy_log_reload(void) {
    nproxy_log_fclose();
    nproxy_log_fopen();
    return;
}

apr_status_t nproxy_log_setup(const char *conf_file) {

    if (!(APR_STATUS_SUCCESS == log_parse_config(conf_file))) {
        return APR_STATUS_ERROR;
    }

    nproxy_log_fopen();

    nn_log_set_function(nproxy_log_function);

    initialized = 1;

    return APR_STATUS_SUCCESS;
}

/* ********************************************************************* */
/* ********************************************************************* */
/* ********************************************************************* */

apr_status_t nproxy_log_init(apr_pool_t * pool, int do_stdout) {

    if (pool == NULL) {
        /* Allocate our main memory pool */
        apr_pool_create_core(&global_pool);
        if (global_pool == NULL) {
            printf("Cannot allocate log memory pool");
            return APR_STATUS_ERROR;
        }
        pool_is_mine = 1;
    } else {
        global_pool = pool;
        pool_is_mine = 0;
    }

    /* Get some memory for our logs */
    globals = apr_pcalloc(global_pool, sizeof(log_setting_t));
    if (globals == NULL) {
        printf("Cannot allocate memory for log settings");
        return APR_STATUS_ERROR;
    }

    globals->always_do_stdout = do_stdout;
    globals->eventlog = NULL;

    initialized = 0;

    return APR_STATUS_SUCCESS;
}

void nproxy_log_destroy(void) {
    nproxy_log_fclose();

    if (pool_is_mine) {
        apr_pool_destroy(global_pool);
        global_pool = NULL;
    }

    return;
}
