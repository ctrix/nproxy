#!/bin/bash

pushd ..

#
# This file is used to conform all code to the same style.
# Please use indent, if you need to submit patches or contribute
# your code.
#

function do_indent() {
    local FROM=$1
    local TO=$2


    /usr/bin/indent \
        --braces-on-func-def-line \
        \
        --no-blank-lines-after-declarations \
        --no-blank-lines-after-procedures \
        --no-blank-lines-after-commas \
        --break-before-boolean-operator \
        --braces-on-struct-decl-line \
        --honour-newlines \
        --braces-on-if-line \
        --comment-indentation33 \
        --declaration-comment-column33 \
        --no-comment-delimiters-on-blank-lines \
        --cuddle-else \
        --continuation-indentation4 \
        --case-indentation0 \
        --line-comments-indentation0 \
        --declaration-indentation1 \
        --dont-format-first-column-comments \
        --parameter-indentation0 \
        --continue-at-parentheses \
        --no-space-after-function-call-names \
        --dont-break-procedure-type \
        --space-after-if \
        --space-after-for \
        --space-after-while \
        --no-space-after-parentheses \
        --dont-star-comments \
        --swallow-optional-blank-lines \
        --dont-format-comments \
        --else-endif-column33 \
        --space-special-semicolon \
        --indent-label1 \
        --case-indentation4 \
        \
        --tab-size4 \
        -i4 \
        -l200 \
        --no-tabs \
        $FROM \
        -o $TO
}


FILES=$( cat <<EOF
lib/libnn/include/nn_buffer.h
lib/libnn/include/nn_log.h
lib/libnn/include/nn_utils.h
lib/libnn/include/nn_xml.h
lib/libnn/nn_buffer.c
lib/libnn/nn_log.c
lib/libnn/nn_utils.c
lib/libnn/nn_xml.c

lib/libnproxy/include/nproxy.h
lib/libnproxy/nproxy-conf.h.in
lib/libnproxy/libnproxy_auth.c
lib/libnproxy/libnproxy.c
lib/libnproxy/libnproxy_conn.c
lib/libnproxy/libnproxy.h
lib/libnproxy/libnproxy_headers.c
lib/libnproxy/libnproxy_lua.c
lib/libnproxy/libnproxy_profiles.c
lib/libnproxy/libnproxy_response.c
lib/libnproxy/libnproxy_swig.c
lib/libnproxy/libnproxy_swig.h
lib/libnproxy/libnproxy_swig.i
lib/libnproxy/libnproxy_swig_wrap.c
lib/libnproxy/libnproxy_utils.c

src/nproxyd.c
src/nproxyd.h.in
src/nproxyd_log.c
EOF
)

for FILE in $FILES; do
    do_indent $FILE $FILE
done
