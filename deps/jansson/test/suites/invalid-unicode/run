#!/bin/sh
#
# Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
#
# Jansson is free software; you can redistribute it and/or modify
# it under the terms of the MIT license. See LICENSE for details.

is_test() {
    test -d $test_path
}

run_test() {
    $json_process --env <$test_path/input >$test_log/stdout 2>$test_log/stderr
    valgrind_check $test_log/stderr || return 1
    cmp -s $test_path/error $test_log/stderr
}

show_error() {
    valgrind_show_error && return

    echo "EXPECTED ERROR:"
    nl -bn $test_path/error
    echo "ACTUAL ERROR:"
    nl -bn $test_log/stderr
}

. $top_srcdir/test/scripts/run-tests.sh
