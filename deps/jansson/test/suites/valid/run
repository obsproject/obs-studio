#!/bin/sh
#
# Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
#
# Jansson is free software; you can redistribute it and/or modify
# it under the terms of the MIT license. See LICENSE for details.

JSON_SORT_KEYS=1
export JSON_SORT_KEYS

is_test() {
    test -d $test_path
}

do_run() {
    variant=$1
    s=".$1"

    strip=0
    [ "$variant" = "strip" ] && strip=1

    STRIP=$strip $json_process --env \
        <$test_path/input >$test_log/stdout$s 2>$test_log/stderr$s
    valgrind_check $test_log/stderr$s || return 1

    ref=output
    [ -f $test_path/output$s ] && ref=output$s

    if ! cmp -s $test_path/$ref $test_log/stdout$s; then
        echo $variant > $test_log/variant
        return 1
    fi
}

run_test() {
    do_run normal && do_run strip
}

show_error() {
    valgrind_show_error && return

    read variant < $test_log/variant
    s=".$variant"

    echo "VARIANT: $variant"

    echo "EXPECTED OUTPUT:"
    ref=output
    [ -f $test_path/output$s ] && ref=output$s
    nl -bn $test_path/$ref

    echo "ACTUAL OUTPUT:"
    nl -bn $test_log/stdout$s
}

. $top_srcdir/test/scripts/run-tests.sh
