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
 * libnproxy_auth.c
 *
 */

#include "libnproxy.h"

/*
   taken from http://www.tug.org/ftp/vm/base64-decode.c, public domain
*/
#define BUFSIZ_B64D 512

char *base64_decode(apr_pool_t * pool, const char *in) {
    unsigned char alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static char inalphabet[256], decoder[256];
    int i;
    int bits, c, char_count, n = 0;
    char buf[BUFSIZ_B64D];

    for (i = (int) (sizeof alphabet) - 1; i >= 0; i--) {
        inalphabet[alphabet[i]] = 1;
        decoder[alphabet[i]] = i;
    }

    char_count = 0;
    bits = 0;

    for (i = 0; i < (int) strlen(in); i++) {
        c = in[i];
        if (c == '=') {
            switch (char_count) {
                    if (!((n + char_count) < BUFSIZ_B64D))
                        return NULL;    /* too big for buffer */
                case 1:
                    /* base64 encoding incomplete: at least 2 bits missing */
                    return NULL;
                    break;
                case 2:
                    buf[n++] = bits >> 10;
                    char_count = 0;
                    break;
                case 3:
                    char_count = 0;
                    buf[n++] = bits >> 16;
                    buf[n++] = (bits >> 8) & 0xff;
                    break;
            }
        }
        if (c > 255 || !inalphabet[c])
            continue;

        bits += decoder[c];
        char_count++;
        if (char_count == 4) {
            if (!((n + 4) < BUFSIZ_B64D))
                return NULL;    /* too big for buffer */
            buf[n++] = bits >> 16;
            buf[n++] = (bits >> 8) & 0xff;
            buf[n++] = bits & 0xff;
            bits = 0;
            char_count = 0;
        } else {
            bits <<= 6;
        }
    }

    if (char_count) {
        return NULL;            /* base64 encoding incomplete */
    }

    if (n == BUFSIZ_B64D) {
        return NULL;            /* just a bit too big for the buffer! */
    }

    buf[n] = '\0';

    return apr_pool_strdup(pool, buf);
}

int basic_challenge_auth(nproxy_connection_t * conn, const char *digest) {
    char *type;
    char *enc;

    if ((enc = strcasestr(digest, "Basic"))) {
        type = enc;
        enc = enc + strlen("Basic");
        while (enc && (*enc == ' ')) {
            enc++;
        }
        *(type + strlen("Basic")) = '\0';

        if (!strncasecmp(type, "Basic", strlen("Basic"))) {
            char *dec;
            dec = base64_decode(conn->pool, enc);
            if (dec) {
                char *user = NULL;
                char *pass = NULL;
                user = dec;
                pass = strchr(user, ':');
                if (user && pass) {
                    *(pass++) = '\0';
                    return lua_call_auth_basic(conn, user, pass);
                }
            }
        } else {
            nn_log(NN_LOG_CRIT, "Auth '%s' not supported.", type);
        }
    }

    nproxy_connection_set_variable(conn, "custom_detail", "Authorization method not supported");

    return HTTP_INTERNAL_SERVER_ERROR;
}
