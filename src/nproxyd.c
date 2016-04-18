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
 * nproxyd.c
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "apr.h"
#include "apr_strings.h"
#include "apr_thread_proc.h"

#include "nproxyd.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

/* Picky compiler */
#ifdef __ICC
#pragma warning (disable:167)
#endif

#include "nn_conf.h"

#include "nn_xml.h"
#include "nn_log.h"
#include "nn_utils.h"

#include "nproxy.h"
#include "nproxyd.h"

/* **************************************************************************
	GLOBAL VARIABLES
   ************************************************************************** */

static apr_pool_t *main_pool;
static int running;
static apr_uint16_t rec_termreq = 0;
static char *runas_user = NULL;
static char *runas_group = NULL;
static int running = 1;
static int foreground = 0;
static int dumpcore = 0;
static char *config_file = NULL;
static char *pid_file = NULL;
static int store = 0;

#ifdef WIN32
#include "nproxyd_win32.c"
#endif

static char *USAGE = "Optional parameters that nproxy may accepts\n" "   -c config file          alternate configuration file\n"
#ifdef WIN32
    "   -store                  store config file path in registry\n"
    "   -service   [name]       start nproxy as a service \n"
    "                           cannot be used if loaded as a console app\n"
    "   -install   [name]       install nproxy as a service\n" "                           with optional service name\n" "   -uninstall [name]       remove nproxy as a service\n"
#else
    "   -f                      stay in foreground. Do not daemonize.\n" "   -u [name]               run as user username.\n" "   -g [name]               run as group groupname.\n"
#ifdef HAVE_RLIMIT
    "   -d                      dump cores\n"
#endif
#endif
    "   -h                      this help\n" "   --help                  this help\n" "\n";

/* **************************************************************************
	SIGNAL HANDLERS
   ************************************************************************** */

static void handle_signals(int sig) {
    if (0) {
    }
#ifdef HAVE_SIGHUP
    else if (sig == SIGHUP) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (HUP). Rotating logs.", sig);
        nproxy_log_reload();
    }
#endif
#ifdef HAVE_SIGTERM
    else if (sig == SIGTERM) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (TERM). Terminating.", sig);
        running = 0;
        rec_termreq++;
    }
#endif
#ifdef HAVE_SIGINT
    else if (sig == SIGINT) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (INT). Terminating.", sig);
        running = 0;
        rec_termreq++;
    }
#endif
#ifdef HAVE_SIGQUIT
    else if (sig == SIGQUIT) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (QUIT)", sig);
        running = 0;
        rec_termreq++;
    }
#endif
#ifdef HAVE_SIGILL
    else if (sig == SIGILL) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (ILL). Terminating.", sig);
        running = 0;
    }
#endif
#ifdef HAVE_SIGBUS
    else if (sig == SIGBUS) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (BUS)", sig);
        running = 0;
    }
#endif
#ifdef HAVE_SIGPOLL
    else if (sig == SIGPOLL) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (POLL)", sig);
    }
#endif
#ifdef HAVE_SIGIO
    else if (sig == SIGIO) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (IO)", sig);
    }
#endif
#ifdef HAVE_SIGPIPE
    else if (sig == SIGPIPE) {
        nn_log(NN_LOG_NOTICE, "Got signal %d (PIPE)", sig);
    }
#endif
    else {
        nn_log(NN_LOG_NOTICE, "Got signal %d. Unmanaged.", sig);
    }

    if (rec_termreq >= 3) {
        nn_log(NN_LOG_WARNING, "Exiting brutally: received %d request terminations", rec_termreq);
        exit(0);
    }

/* Re-enable - Windows must re add the signal handler. */
#ifdef WIN32
    signal(sig, handle_signals);
#endif
    return;
}

static void signals_setup(void) {
#ifdef HAVE_SIGHUP
    signal(SIGHUP, handle_signals);
#endif
#ifdef HAVE_SIGINT
    signal(SIGINT, handle_signals); /* Ctrl-c */
#endif
#ifdef HAVE_SIGTERM
    signal(SIGTERM, handle_signals);
#endif
#ifdef HAVE_SIGPIPE
    signal(SIGPIPE, handle_signals);
#endif
#ifdef HAVE_SIGBUS
    signal(SIGBUS, handle_signals);
#endif
#ifdef HAVE_SIGIO
    signal(SIGIO, handle_signals);
#endif
#ifdef HAVE_SIGPOLL
    signal(SIGPOLL, handle_signals);
#endif
#ifdef HAVE_SIGQUIT
    signal(SIGQUIT, handle_signals);
#endif
#ifdef HAVE_SIGILL
    signal(SIGILL, handle_signals);
#endif
}

#ifndef WIN32
static void daemonize(apr_pool_t * pool) {
    pid_t sid;
    FILE *fd;
    apr_proc_t child;
    apr_status_t fork_status;

    fork_status = apr_proc_fork(&child, pool);

    switch (fork_status) {
        case APR_INCHILD:
            /* Do nothing */
            break;
        case APR_INPARENT:
            fprintf(stderr, "Backgrounding.\n");
            exit(EXIT_SUCCESS);
            break;
        default:
            /* Error ... */
            fprintf(stderr, "Error backgrounding NProxy.\n");
            break;
    }

    fork_status = apr_proc_fork(&child, pool);

    switch (fork_status) {
        case APR_INCHILD:
            /* Close file handles */
            fd = freopen("/dev/null", "r", stdin);
            fd = freopen("/dev/null", "w", stdout);
            fd = freopen("/dev/null", "w", stderr);
            (void) fd;
            break;
        case APR_INPARENT:
            exit(EXIT_SUCCESS);
            break;
        default:
            /* Error ... */
            fprintf(stderr, "Error backgrounding NProxy...\n");
            break;
    }

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        fprintf(stderr, "Cannot setsif() the child process\n");
        return;
    }

    return;
}

static void create_pidfile(apr_pool_t * pool, const char *pid_path) {
    pid_t pid;
    apr_file_t *fd;
    apr_size_t flen;

    pid = getpid();

    if (apr_file_open(&fd, pid_path, APR_FOPEN_WRITE | APR_FOPEN_CREATE, APR_FPROT_UREAD | APR_FPROT_UWRITE, pool) == APR_STATUS_SUCCESS) {
        char *pid_c;

        if (apr_file_lock(fd, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK) != APR_STATUS_SUCCESS) {
            fprintf(stderr, "Cannot lock pid file %s.\n", pid_path);
            exit(250);
            return;
        }

        apr_file_trunc(fd, 0);

        pid_c = apr_psprintf(pool, "%d", (int) pid);
        flen = strlen(pid_c);
        apr_file_write(fd, pid_c, &flen);

    } else {
        fprintf(stderr, "Cannot create pid file %s.\n", pid_path);
        exit(250);
    }

    return;
}

static apr_status_t change_user_group(const char *uid, const char *gid) {
    struct group *gr;
    struct passwd *pw;
    uid_t id_u;
    gid_t id_g;

    /* Check if gid exists */
    gr = getgrnam(gid);
    if (!gr) {
        fprintf(stderr, "No such group '%s'!\n", gid);
        return APR_STATUS_ERROR;
    }

    /* Check id uid exists */
    pw = getpwnam(uid);
    if (!pw) {
        fprintf(stderr, "No such user '%s'!\n", uid);
        return APR_STATUS_ERROR;
    }

    id_u = pw->pw_uid;
    id_g = gr->gr_gid;

    if (id_u == 0) {
        fprintf(stderr, "Cannot run as root!\n");
        return APR_STATUS_ERROR;
    }

    if ((id_u && getuid() == id_u) && (!id_g || id_g == getgid())) {
        /* already running as the right user and group, nothing to do! */
        return APR_STATUS_SUCCESS;
    }

    /* Drop initial privileges */
    if (setgroups(0, NULL) < 0) {
        fprintf(stderr, "Failed to drop group access list\n");
        return APR_STATUS_ERROR;
    }

    /* Setting user privileges. */
    if (gr->gr_gid != getegid()) {
        if (initgroups(pw->pw_name, gr->gr_gid) == -1) {
            nn_log(NN_LOG_ERROR, "Unable to initgroups '%s' (%d)\n", pw->pw_name, gr->gr_gid);
            fprintf(stderr, "Unable to initgroups '%s' (%d)\n", pw->pw_name, gr->gr_gid);
            return APR_STATUS_ERROR;
        }
    }

    if (setregid(gr->gr_gid, gr->gr_gid)) {
        nn_log(NN_LOG_ERROR, "Unable to setgid to '%s' (%d)\n", gr->gr_name, gr->gr_gid);
        fprintf(stderr, "Unable to setgid to '%s' (%d)\n", gr->gr_name, gr->gr_gid);
        return APR_STATUS_ERROR;
    }
#ifdef __APPLE__
    if (seteuid(pw->pw_uid)) {
#else
    if (setreuid(pw->pw_uid, pw->pw_uid)) {
#endif
        nn_log(NN_LOG_ERROR, "Unable to setuid to '%s' (%d)\n", pw->pw_name, pw->pw_uid);
        fprintf(stderr, "Unable to setuid to '%s' (%d)\n", pw->pw_name, pw->pw_uid);
        return APR_STATUS_ERROR;
    }

    /* Check if we're root */
    if (!geteuid()) {
        nn_log(NN_LOG_ERROR, "Running as root is not allowed.\n");
        fprintf(stderr, "Running as root is not allowed\n");
        return APR_STATUS_ERROR;
    }

    return APR_STATUS_SUCCESS;
}

#endif

/* **************************************************************************
	COMMAND LINE
   ************************************************************************** */

static apr_status_t parse_command_line(int argc, char *argv[]) {
    int x;

    for (x = 1; x < argc; x++) {

        if (argv[x] && !strcmp(argv[x], "-h")) {
            printf("%s", USAGE);
            exit(0);
        }

        if (argv[x] && !strcmp(argv[x], "--help")) {
            printf("%s", USAGE);
            exit(0);
        }
#ifdef WIN32
        if (argv[x] && !strcmp(argv[x], "-store")) {
            store = 1;
        }

        if (argv[x] && !strcmp(argv[x], "-service")) {
            nproxy_win32_service_start();
        }

        if (argv[x] && !strcmp(argv[x], "-install")) {
            char *sname = NULL;
            x++;
            if (argv[x] && strlen(argv[x])) {
                strncpy(service_name, argv[x], SERVICENAME_MAXLEN);
            } else {
                strncpy(service_name, SERVICENAME_DEFAULT, SERVICENAME_MAXLEN);
            }
            nproxy_win32_service_install();
        }

        if (argv[x] && !strcmp(argv[x], "-uninstall")) {
            char *sname = NULL;
            x++;
            if (argv[x] && strlen(argv[x])) {
                strncpy(service_name, argv[x], SERVICENAME_MAXLEN);
            } else {
                strncpy(service_name, SERVICENAME_DEFAULT, SERVICENAME_MAXLEN);
            }
            nproxy_win32_service_uninstall();
        }
#else
        store = 0;

        if (argv[x] && !strcmp(argv[x], "-f")) {
            foreground = 1;
        }

        if (argv[x] && !strcmp(argv[x], "-u")) {
            x++;
            if (argv[x] && strlen(argv[x])) {
                runas_user = strdup(argv[x]);
            } else {
                nn_log(NN_LOG_ERROR, "You must provide a user name when using -u.");
                exit(251);
            }
        }

        if (argv[x] && !strcmp(argv[x], "-g")) {
            x++;
            if (argv[x] && strlen(argv[x])) {
                runas_group = strdup(argv[x]);
            } else {
                nn_log(NN_LOG_ERROR, "You must provide a group name when using -g.");
                exit(250);
            }
        }
#endif

#ifdef HAVE_RLIMIT
        if (argv[x] && !strcmp(argv[x], "-d")) {
            dumpcore = 1;
        }
#endif

        if (argv[x] && !strcmp(argv[x], "-c")) {
            x++;

            if (!zstr(argv[x])) {
                if (apr_file_exists(main_pool, argv[x]) != APR_STATUS_SUCCESS) {
                    nn_log(NN_LOG_ERROR, "We have detected problems with the config file. It does not exists: %s", argv[x]);
                    exit(253);
                }

                if (apr_is_file(main_pool, argv[x]) != APR_STATUS_SUCCESS) {
                    nn_log(NN_LOG_ERROR, "We have detected problems with the config file. This isn't a regular file: %s", argv[x]);
                    exit(252);
                }
#ifndef WIN32
                if (argv[x][0] != '/') {
                    nn_log(NN_LOG_ERROR, "You must provide an absolute path for the alternate config file.");
                    exit(252);
                }
#endif
                config_file = strdup(argv[x]);

                nn_log(NN_LOG_NOTICE, "Config file override: %s", config_file);
            } else {
                fprintf(stderr, "When using -c you must specify a config file\n");
                exit(255);
            }
        }
    }
    return APR_STATUS_SUCCESS;
}

/* **************************************************************************
	PROXY FUNCTIONS
   ************************************************************************** */

typedef struct profile_task_s profile_task_t;
struct profile_task_s {
    nproxy_profile_t *profile;
    apr_thread_t *thread;
    profile_task_t *next;
};

static profile_task_t *profile_tasks = NULL;
static apr_pool_t *threads_pool = NULL;

static void *APR_THREAD_FUNC profile_thread_run(apr_thread_t * thread, void *data) {
    nproxy_profile_t *prof;

    prof = data;
    nproxy_profile_run(prof);

    apr_thread_exit(thread, APR_STATUS_SUCCESS);
    return NULL;
}

static apr_status_t nproxy_run_profile(apr_pool_t * pool, profile_task_t * task) {
    apr_thread_t *thread;
    nproxy_profile_t *profile;

    profile = task->profile;

    if (nproxy_profile_start(profile) != APR_STATUS_SUCCESS) {
        return APR_STATUS_ERROR;
    }

    if (apr_thread_create(&thread, NULL, profile_thread_run, profile, pool) != APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Profile '%s': cannot start thread", nproxy_profile_get_name(profile));
        return APR_STATUS_ERROR;
    }

    task->thread = thread;
    nn_log(NN_LOG_NOTICE, "Profile '%s': thread started", nproxy_profile_get_name(profile));

    return APR_STATUS_SUCCESS;
}

static apr_status_t nproxy_run_profiles(void) {
    nproxy_profile_t *profile = NULL;
    profile_task_t *task = NULL;
    nn_xml_t x_config = NULL, x_profiles = NULL, x_profile = NULL, x_param = NULL;

    if (nproxy_init("test server") != APR_STATUS_SUCCESS) {
        return APR_STATUS_ERROR;
    }

    apr_pool_create_core(&threads_pool);
    if (threads_pool == NULL) {
        return APR_STATUS_ERROR;
    }

    if ((x_config = nn_xml_parse_file(config_file)) != NULL) {
        if ((x_profiles = nn_xml_child(x_config, "profiles")) != NULL) {
            for (x_profile = nn_xml_child(x_profiles, "profile"); x_profile; x_profile = x_profile->next) {
                char *pname = (char *) nn_xml_attr_soft(x_profile, "name");
                apr_status_t ok;
                char *l_ip = "";
                char *b_ip = NULL;
                int l_port = 8080;
                int in_timeout = 60;
                int max_duration = 0;
                int t_interval = 500;
                int io_interval = 1000;
                size_t shape_after = 0;
                size_t shape_bps = 0;
                char *template_dir = NULL;
                char *script_dir = NULL;
                char *script = NULL;
                int prefer_v6 = 0;

                nn_log(NN_LOG_NOTICE, "Starting profile '%s'", pname);

                task = apr_pcalloc(threads_pool, sizeof(profile_task_t));
                if (task == NULL) {
                    nn_log(NN_LOG_NOTICE, "Error allocatin memory for profile '%s'", pname);
                    return APR_STATUS_ERROR;
                }

                profile = nproxy_profile_create(pname);
                if (!profile) {
                    nn_log(NN_LOG_NOTICE, "Error allocatin memory for profile '%s'.", pname);
                    return APR_STATUS_ERROR;
                }

                for (x_param = nn_xml_child(x_profile, "param"); x_param; x_param = x_param->next) {
                    char *name = (char *) nn_xml_attr_soft(x_param, "name");
                    char *value = (char *) nn_xml_attr_soft(x_param, "value");
                    if (!strlen(name) || !strlen(value)) {
                        continue;
                    } else if (!strncmp(name, "listen_address", strlen("listen_address") + 1)) {
                        l_ip = value;
                    } else if (!strncmp(name, "listen_port", strlen("listen_port") + 1)) {
                        l_port = atoi(value);
                    } else if (!strncmp(name, "bind_address", strlen("bind_address") + 1)) {
                        b_ip = value;
                    } else if (!strncmp(name, "prefer_ipv6", strlen("prefer_ipv6") + 1)) {
                        prefer_v6 = nproxyd_is_true(value);
                    } else if (!strncmp(name, "inactivity_timeout", strlen("inactivity_timeout") + 1)) {
                        in_timeout = atoi(value);
                    } else if (!strncmp(name, "max_duration", strlen("max_duration") + 1)) {
                        max_duration = atoi(value);
                    }

                    else if (!strncmp(name, "shape_after", strlen("shape_after") + 1)) {
                        shape_after = atoi(value);
                    } else if (!strncmp(name, "shape_bps", strlen("shape_bps") + 1)) {
                        shape_bps = atoi(value);
                    } else if (!strncmp(name, "template_dir", strlen("template_dir") + 1)) {
                        template_dir = value;
                    } else if (!strncmp(name, "script_dir", strlen("script_dir") + 1)) {
                        script_dir = value;
                    } else if (!strncmp(name, "script_file", strlen("script_file") + 1)) {
                        script = value;
                    }

                    else {
                        nn_log(NN_LOG_ERROR, "Ignoring parameter '%s' for profile '%s'", name, pname);
                    }
                }

                if (l_port <= 0) {
                    l_port = 8080;
                }
                if (max_duration < 0) {
                    max_duration = 0;
                }
                if (in_timeout < 0) {
                    in_timeout = 0;
                }
                if (io_interval < 100) {
                    io_interval = 100;
                }
                if (io_interval > 2000) {
                    io_interval = 2000;
                }
                if (t_interval < 100) {
                    t_interval = 100;
                }
                if (t_interval > 2000) {
                    t_interval = 2000;
                }

                nproxy_profile_set_v6_preference(profile, prefer_v6);
                nproxy_profile_set_listen(profile, l_ip, l_port);
                nproxy_profile_set_bind(profile, b_ip);
                nproxy_profile_set_inactivity_timeout(profile, in_timeout);
                nproxy_profile_set_max_duration(profile, max_duration);
                nproxy_profile_set_shaper(profile, shape_after, shape_bps);

                if (!zstr(template_dir) && (apr_file_exists(threads_pool, template_dir) == APR_STATUS_SUCCESS) && (apr_is_dir(threads_pool, template_dir) == APR_STATUS_SUCCESS)) {
                    nproxy_profile_set_template_dir(profile, template_dir);
                } else {
                    if (!zstr(template_dir)) {
                        nn_log(NN_LOG_WARNING, "Profile '%s': invalid template_dir (%s)", pname, template_dir);
                    }
                }

                if (!zstr(script_dir) && (apr_file_exists(threads_pool, script_dir) == APR_STATUS_SUCCESS) && (apr_is_dir(threads_pool, script_dir) == APR_STATUS_SUCCESS)) {
                    nproxy_profile_set_script_dir(profile, script_dir);
                } else {
                    if (!zstr(script_dir)) {
                        nn_log(NN_LOG_WARNING, "Profile '%s': invalid script_dir (%s)", pname, script_dir);
                    }
                }
                nproxy_profile_set_luascript(profile, script);
                /*
                   TODO
                   nproxy_profile_set_maxclient(profile, 100);
                 */

                task->profile = profile;
                ok = nproxy_run_profile(threads_pool, task);
                if (ok == APR_STATUS_SUCCESS) {
                    task->next = profile_tasks;
                    profile_tasks = task;
                } else {
                    nproxy_profile_stop(profile);
                    nproxy_profile_destroy(profile);
                    nn_log(NN_LOG_ERROR, "Profile '%s' not started", pname);
                }

            }
        }
    }

    if (x_config) {
        nn_xml_free(x_config);
    }

    return APR_STATUS_SUCCESS;
}

static apr_status_t nproxy_stop_profile(profile_task_t * task) {
    nproxy_profile_t *profile;
    apr_thread_t *thread;
    apr_status_t ret;

    profile = task->profile;
    thread = task->thread;

    nproxy_profile_stop(profile);

    if (apr_thread_join(&ret, thread) == APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_NOTICE, "Profile '%s': thread exited", nproxy_profile_get_name(profile));
        ret = APR_STATUS_SUCCESS;
    } else {
        nn_log(NN_LOG_ERROR, "Profile '%s': thread didn't exit successfully", nproxy_profile_get_name(profile));
        ret = APR_STATUS_ERROR;
    }

    nproxy_profile_destroy(profile);

    return ret;
}

static void nproxy_stop_profiles(void) {
    profile_task_t *task;

    task = profile_tasks;
    while (task) {
        nproxy_stop_profile(task);
        task = task->next;
    }

    nproxy_deinit();
    apr_pool_destroy(threads_pool);
}

/* **************************************************************************
	INITIALIZATIONS
   ************************************************************************** */

static int setup(void) {

    if (nproxy_log_init(NULL, foreground) != APR_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot initialize logging infrastructure\n");
        exit(1);
    }

    /* Now set up the proper logging engine */
    if (nproxy_log_setup(config_file) != APR_STATUS_SUCCESS) {
        fprintf(stderr, "Cannot set up logging options\n");
        exit(1);
    }

    /* Set up the signal handlers */
    signals_setup();

#ifdef HAVE_RLIMIT
    if (dumpcore) {
        struct rlimit l;
        memset(&l, 0, sizeof(l));
        l.rlim_cur = RLIM_INFINITY;
        l.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &l)) {
            nn_log(NN_LOG_WARNING, "Unable to disable core size resource limit: %s", strerror(errno));
        } else {
            nn_log(NN_LOG_NOTICE, "Setting unlimited core size.");
        }
    }
#endif

    /* Let's change our UMASK */
#if defined(HAVE_SYS_STAT_H) && defined(HAVE_UMASK)
#ifndef WIN32
    umask(0177);
#else
    {
        int oldmask;
        _umask_s(_S_IWRITE | _S_IREAD, &oldmask);
    }
#endif
#endif

    return 1;
}

int main(int argc, char **argv) {
    dumpcore = 0;               /* Initialize it so that no compiler throws useless warnings. */

    apr_initialize();

    apr_pool_create_core(&main_pool);
    if (main_pool == NULL) {
        fprintf(stderr, "Cannot initialize main memory pool. Sorry.\n");
        exit(1);
    }

    /* Parsing CLI parameters */
    if ((parse_command_line(argc, argv) != APR_STATUS_SUCCESS)) {
        exit(1);
    }

    /* Check our config file */
    if (!config_file) {
        char cf[2048] = "";
#ifdef WIN32
        //TODO
        if (LoadRegValue("ConfigFile", cf, sizeof(cf)) != APR_STATUS_SUCCESS) {
            nn_log(NN_LOG_ERROR, "Config file not found. Use -c (and possibly -store)");
            exit(1);
        }

        if (!strlen(cf)) {
            nn_log(NN_LOG_ERROR, "Config file not found. Use -c (and possibly -store)");
            exit(1);
        }

        nn_log(NN_LOG_DEBUG, "Using config file from registry: %s", config_file);
#else
        snprintf(cf, sizeof(cf), "%s" APR_PATH_SEPARATOR "%s", NPROXY_CONFIG_DIR, NPROXY_CONFIG_FILE);
        nn_log(NN_LOG_DEBUG, "Using default config file path: %s", cf);
#endif
        config_file = strdup(cf);
    } else {
#ifdef WIN32
        if (store) {
            if (SaveRegValue("ConfigFile", config_file) == APR_STATUS_SUCCESS) {
                nn_log(NN_LOG_NOTICE, "Storing config file path to registry.");
            } else {
                nn_log(NN_LOG_NOTICE, "Cannot store config file path to registry.");
            }
        }
#endif
    }

    setup();

    /* Prepare to daemonize. */
    if (foreground == 0) {
#ifndef WIN32
        daemonize(main_pool);
#else
        FreeConsole();
#endif
    }

    /* PID file creation */
#ifndef WIN32
    pid_file = apr_pcalloc(main_pool, strlen(NPROXY_STATE_DIR) + strlen("nproxy.pid") + 2);
    if (pid_file != NULL) {
        snprintf(pid_file, strlen(NPROXY_STATE_DIR) + strlen("nproxy.pid") + 2, "%s/nproxy.pid", NPROXY_STATE_DIR);
        create_pidfile(main_pool, pid_file);
    }
#endif

    if (nproxy_run_profiles() != APR_STATUS_SUCCESS) {
        nn_log(NN_LOG_ERROR, "Error seting up proxy profiles");
        goto done;
    }

    if (!profile_tasks) {
        nn_log(NN_LOG_ERROR, "No profiles were started. Shutting down.");
        fprintf(stderr, "No profiles were started. Shutting down.\n");
        goto done;
    }
    /* Setup user/group once we opened our sockets */

#ifndef WIN32
    if (change_user_group((runas_user) ? runas_user : NPROXY_DEFAULT_USER, (runas_group) ? runas_group : NPROXY_DEFAULT_GROUP) != APR_STATUS_SUCCESS) {
        exit(240);
    }
#endif

    /* chdir to our home */
#ifndef WIN32
    {
        char *home;
        char *uname;
        apr_uid_t userid;
        apr_gid_t groupid;

        apr_uid_current(&userid, &groupid, main_pool);
        apr_uid_name_get(&uname, userid, main_pool);
        apr_uid_homepath_get(&home, uname, main_pool);

        if (chdir(home)) {
            fprintf(stderr, "Cannot change Working directory to '%s'\n", home);
            exit(230);
        }
    }
#endif

    while (running) {
        apr_sleep(APR_USEC_PER_SEC);
    }

 done:
    nproxy_stop_profiles();

#ifndef WIN32
    /* Remove our pid file */
    if (!zstr(pid_file)) {
        apr_file_remove(pid_file, main_pool);
    }
#endif

    nn_log(NN_LOG_ERROR, "Nproxy stopped.");

    nproxy_log_destroy();
    apr_pool_destroy(main_pool);
    apr_terminate();

    apr_safe_free(config_file);
    return 0;
}
