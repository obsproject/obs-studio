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
    (
        if [ -f $test_path/env ]; then
            . $test_path/env
        fi
        $json_process --env <$test_path/input >$test_log/stdout 2>$test_log/stderr
    )
    valgrind_check $test_log/stderr || return 1
    cmp -s $test_path/output $test_log/stdout
}

show_error() {
    valgrind_show_error && return

    echo "EXPECTED OUTPUT:"
    nl -bn $test_path/output
    echo "ACTUAL OUTPUT:"
    nl -bn $test_log/stdout
}

. $top_srcdir/test/scripts/run-tests.sh
