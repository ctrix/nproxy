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
 * libnproxy_response.c
 *
 */

#include <time.h>
#include "libnproxy.h"

static struct http_error_codes_s {
    http_resp_codes_t code;
    int number;
    char *string;
} ec[] = {
    {
    HTTP_CONTINUE, 100, "Continue"}, {
    HTTP_OK, 200, "Ok"}, {
    HTTP_CREATED, 201, "Created"}, {
    HTTP_ACCEPTED, 202, "Accepted"}, {
    HTTP_NO_CONTENT, 204, "No Content"}, {
    HTTP_RESET_CONTENT, 205, "Reset Content"}, {
    HTTP_PARTIAL_CONTENT, 206, "Partial Content"}, {
    HTTP_MULTIPLE_CH, 300, "Multiple Choices"}, {
    HTTP_MOVED_PERM, 301, "Moved Permanently"}, {
    HTTP_MOVED_TEMP, 302, "Moved Temporarily"}, {
    HTTP_NOT_MODIFIED, 304, "Not Modified"}, {
    HTTP_USE_PROXY, 305, "Use Proxy"}, {
    HTTP_BAD_REQUEST, 400, "Bad Request"}, {
    HTTP_UNAUTHORIZED, 401, "Unauthorized"}, {
    HTTP_FORBIDDEN, 403, "Forbidden"}, {
    HTTP_NOT_FOUND, 404, "Not Found"}, {
    HTTP_NOT_ALLOWED, 405, "Method Not Allowed"}, {
    HTTP_NOT_ACCEPTABLE, 406, "Not Acceptable"}, {
    HTTP_PROXY_AUTH_REQ, 407, "Proxy Authentication Required"}, {
    HTTP_TIMEOUT, 408, "Request Time-out"}, {
    HTTP_CONFLICT, 409, "Conflict"}, {
    HTTP_GONE, 410, "Gone"}, {
    HTTP_INTERNAL_SERVER_ERROR, 500, "Internal Server Error"}, {
    HTTP_NOT_IMPLEMENTED, 501, "Not Implemented"}, {
    HTTP_BAD_GATEWAY, 502, "Bad Gateway"}, {
    HTTP_SERVICE_UNAVALAIBLE, 503, "Service Unavailable"}, {
HTTP_VERSION_NOT_SUPPORTED, 505, "HTTP Version not supported"},};

static char *find_template(nproxy_connection_t * conn, int code, const char *suggested) {
    char *file = NULL;
    char *tpl_dir = NULL;

    tpl_dir = conn->profile->template_dir;

    if (suggested != NULL) {
        if (!tpl_dir) {
            file = (char *) suggested;
            if ((apr_file_exists(conn->pool, file) == APR_STATUS_SUCCESS) && (apr_is_file(conn->pool, file) == APR_STATUS_SUCCESS)) {
                return file;
            }
        } else {
            file = apr_psprintf(conn->pool, "%s/%s", tpl_dir, suggested);
            if ((apr_file_exists(conn->pool, file) == APR_STATUS_SUCCESS) && (apr_is_file(conn->pool, file) == APR_STATUS_SUCCESS)) {
                return file;
            }
        }

        nn_log(NN_LOG_WARNING, "Cannot find template file (%s)", suggested);
    }

    if (tpl_dir == NULL) {
        file = apr_psprintf(conn->pool, "%d.html", code);
        if ((apr_file_exists(conn->pool, file) == APR_STATUS_SUCCESS) && (apr_is_file(conn->pool, file) == APR_STATUS_SUCCESS)) {
            return file;
        }

        file = "default.html";
        if ((apr_file_exists(conn->pool, file) == APR_STATUS_SUCCESS) && (apr_is_file(conn->pool, file) == APR_STATUS_SUCCESS)) {
            return file;
        }
    } else {
        file = apr_psprintf(conn->pool, "%s/%d.html", tpl_dir, code);
        if ((apr_file_exists(conn->pool, file) == APR_STATUS_SUCCESS) && (apr_is_file(conn->pool, file) == APR_STATUS_SUCCESS)) {
            return file;
        }

        file = apr_psprintf(conn->pool, "%s/default.html", tpl_dir);
        if ((apr_file_exists(conn->pool, file) == APR_STATUS_SUCCESS) && (apr_is_file(conn->pool, file) == APR_STATUS_SUCCESS)) {
            return file;
        }
    }

    return NULL;
}

static void do_subst(const char *in, nn_buffer_t * buff, const char *s1, const char *s2) {
    const char *p1 = NULL;
    char *p2 = NULL;

    if (!in || !s1 || !s2) {
        return;
    }

    p1 = in;

    while ((p1 <= in + strlen(in)) && (p2 = strstr(p1, s1))) {
        int len = p2 - p1;
        if (len) {
            nn_buffer_append(buff, p1, len);
        }
        nn_buffer_append(buff, s2, strlen(s2));
        p1 = p2 + strlen(s1);
    }

    if (p1) {
        nn_buffer_append(buff, p1, strlen(p1));
    }

    return;
}

static char *parse_template(nproxy_connection_t * conn, const char *file) {
    apr_file_t *f;
    char b[4096];
    apr_size_t blen;
    apr_status_t status;
     /**/ char *in_p = NULL;
    nn_buffer_t *bufferin;
    size_t out_size;
    char *out_p = NULL;
    nn_buffer_t *bufferout;

    bufferin = nn_buffer_init(conn->pool, 0, 4096);
    if (!bufferin) {
        return NULL;
    }

    bufferout = nn_buffer_init(conn->pool, 0, 4096);
    if (bufferout == NULL) {
        return NULL;
    }

    if (APR_STATUS_SUCCESS != apr_file_open(&f, file, APR_READ, 0, conn->pool)) {
        nn_log(NN_LOG_ERROR, "Cannot open template file %s", file);
        return NULL;
    }

    do {
        blen = sizeof(b);
        status = apr_file_read(f, b, &blen);
        if (blen > 0) {
            if (nn_buffer_append(bufferin, b, blen) != APR_STATUS_SUCCESS) {
                return NULL;
            }
        }
    } while (blen > 0);
    apr_file_close(f);

    if (status != APR_EOF) {
        /* An error occurred reading the file */
        nn_log(NN_LOG_ERROR, "Error reading template file %s", file);
        return NULL;
    }

    /* Now we have the full content of the file. We must start evaluating our substitutions. */
    nn_buffer_get_char(bufferin, &in_p);
    if (in_p == NULL) {
        return NULL;
    }

    if (conn->var_hash) {
        apr_hash_index_t *hi;

        for (hi = apr_hash_first(conn->pool, conn->var_hash); hi; hi = apr_hash_next(hi)) {
            apr_ssize_t vlen;
            const char *var = NULL;
            char *val = NULL;

            apr_hash_this(hi, (const void **) &var, &vlen, (void *) &val);

            if (!zstr(var) && !zstr(val)) {
                char *s1 = NULL;

                s1 = apr_psprintf(conn->pool, "{%s}", var);

                nn_buffer_get_char(bufferin, &in_p);
                if (in_p) {
                    nn_buffer_reset(bufferout);
                    do_subst(in_p, bufferout, s1, val);

                    out_size = nn_buffer_get_char(bufferout, &out_p);
                    if (out_size && out_p) {
                        nn_buffer_reset(bufferin);
                        nn_buffer_append(bufferin, out_p, out_size);
                    }
                }
            }

        }
    }

    /* We won't destroy the buffer, it will be deleted with the pool */
    nn_buffer_get_char(bufferin, &in_p);

    return in_p;
}

apr_status_t http_send_custom_response(nproxy_connection_t * conn, http_resp_codes_t error_code) {
    size_t t;
    int clen;
    int e_code;
    char *tpl = NULL;
    char *e_msg = NULL;
    char *e_text = NULL;
    char *e_detail = NULL;
    char *payload = NULL;
    char found = 0;
    char timebuf[APR_RFC822_DATE_LEN];
    char *additional_headers = "";
    const char *tmp = NULL;
    nproxy_connection_side_t *side;
    apr_status_t rv;
    apr_size_t len;

    const char *fallback_error =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
        "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
        "<html>\n" "<head><title>%d %s</title></head>\n" "<body>\n" "<h1>%d %s</h1>\n" "<p>%s</p>\n" "<hr />\n" "<p><em>Generated by NProxy</em></p>\n" "</body>\n" "</html>\n";

    side = conn->browser;

    apr_rfc822_date(timebuf, apr_time_now());

    for (t = 0; t < sizeof(ec) / sizeof(ec[0]); t++) {
        if (error_code == ec[t].code) {
            e_code = ec[t].number;
            e_msg = ec[t].string;
            found = 1;
            break;
        }
    }

    if (!found) {
        e_msg = "Error code string not found.";
        e_code = error_code;
    }

    e_detail = "No details, sorry.";

    if ((tmp = nproxy_connection_get_variable(conn, "additional_headers"))) {
        if (!zstr(tmp)) {
            additional_headers = (char *) tmp;
        }
    }
    if ((tmp = nproxy_connection_get_variable(conn, "custom_cause"))) {
        if (!zstr(tmp)) {
            e_msg = (char *) tmp;
        }
    }
    if ((tmp = nproxy_connection_get_variable(conn, "custom_detail"))) {
        if (!zstr(tmp)) {
            e_detail = (char *) tmp;
        }
    }

    tmp = nproxy_connection_get_variable(conn, "custom_template");
    tpl = find_template(conn, e_code, tmp);

    if (tpl != NULL) {
        if (conn->requests != NULL) {
            nproxy_connection_set_variable(conn, "req_method", conn->requests->method);
            nproxy_connection_set_variable(conn, "req_host", conn->requests->host);
            nproxy_connection_set_variable(conn, "req_port", apr_psprintf(conn->pool, "%d", conn->requests->port));
            nproxy_connection_set_variable(conn, "req_url", conn->requests->url);
        }
        nproxy_connection_set_variable(conn, "resp_errno", apr_psprintf(conn->pool, "%d", e_code));
        nproxy_connection_set_variable(conn, "resp_cause", e_msg);
        nproxy_connection_set_variable(conn, "resp_detail", e_detail);
        nproxy_connection_set_variable(conn, "package", "NProxy");
        e_text = parse_template(conn, tpl);
    }

    if (zstr(e_text)) {
        e_text = apr_psprintf(conn->pool, fallback_error, e_code, e_msg, e_code, e_msg, e_detail);
    }

    clen = strlen(e_text);

    payload = apr_psprintf(conn->pool,
                           "%s %d %s\r\n"
                           "Content-Type: text/html\r\n"
                           "Connection: close\r\n"
                           "Proxy-Connection: close\r\n" "Content-Length: %d\r\n" "Date: %s\r\n" "%s" "\r\n" "%s", "HTTP/1.0", e_code, e_msg, clen, timebuf, additional_headers, e_text);

    len = strlen(payload);
    rv = apr_socket_send(side->sock, payload, &len);
    if (rv == APR_STATUS_SUCCESS) {
        if (conn->requests != NULL) {
            conn->requests->bytes_in += len;
            conn->requests->response_headers.resp_code = error_code;
        }
    } else {
        rv = APR_STATUS_ERROR;
    }

    return rv;
}
