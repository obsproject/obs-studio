#!/bin/sh
#
# Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
#
# Jansson is free software; you can redistribute it and/or modify
# it under the terms of the MIT license. See LICENSE for details.

is_test() {
    case "$test_name" in
        *.c|check-exports)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

run_test() {
    if [ "$test_name" = "check-exports" ]; then
        test_log=$test_log $test_path >$test_log/stdout 2>$test_log/stderr
    else
        $test_runner $suite_builddir/${test_name%.c} \
            >$test_log/stdout \
            2>$test_log/stderr \
            || return 1
        valgrind_check $test_log/stderr || return 1
    fi
}

show_error() {
    valgrind_show_error && return
    cat $test_log/stderr
}

. $top_srcdir/test/scripts/run-tests.sh
