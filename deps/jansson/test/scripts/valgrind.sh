# Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
#
# Jansson is free software; you can redistribute it and/or modify
# it under the terms of the MIT license. See LICENSE for details.

[ -z "$VALGRIND" ] && VALGRIND=0

VALGRIND_CMDLINE="valgrind --leak-check=full --show-reachable=yes --track-origins=yes -q"

if [ $VALGRIND -eq 1 ]; then
    test_runner="$VALGRIND_CMDLINE"
    json_process="$VALGRIND_CMDLINE $json_process"
else
    test_runner=""
fi

valgrind_check() {
    if [ $VALGRIND -eq 1 ]; then
        # Check for Valgrind error output. The valgrind option
        # --error-exitcode is not enough because Valgrind doesn't
        # think unfreed allocs are errors.
        if grep -E -q '^==[0-9]+== ' $1; then
            touch $test_log/valgrind_error
            return 1
        fi
    fi
}

valgrind_show_error() {
    if [ $VALGRIND -eq 1 -a -f $test_log/valgrind_error ]; then
        echo "valgrind detected an error"
        return 0
    fi
    return 1
}
