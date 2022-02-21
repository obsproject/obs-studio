# Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
#
# Jansson is free software; you can redistribute it and/or modify
# it under the terms of the MIT license. See LICENSE for details.

die() {
    echo "$1" >&2
    exit 1
}

[ -n "$1" ] || die "Usage: $0 suite-name"
[ -n "$bindir" ] || die "Set bindir"
[ -n "$logdir" ] || die "Set logdir"
[ -n "$scriptdir" ] || die "Set scriptdir"
[ -n "$suites_srcdir" ] || die "Set suites_srcdir"
[ -n "$suites_builddir" ] || die "Set suites_builddir"

json_process=$bindir/json_process

suite_name=$1
suite_srcdir=$suites_srcdir/$suite_name
suite_builddir=$suites_builddir/$suite_name
suite_log=$logdir/$suite_name

[ -z "$VERBOSE" ] && VERBOSE=0
[ -z "$STOP" ] && STOP=0

. $scriptdir/valgrind.sh

rm -rf $suite_log
mkdir -p $suite_log

for test_path in $suite_srcdir/*; do
    test_name=$(basename $test_path)
    test_builddir=$suite_builddir/$test_name
    test_log=$suite_log/$test_name

    [ "$test_name" = "run" ] && continue
    is_test || continue

    rm -rf $test_log
    mkdir -p $test_log
    if [ $VERBOSE -eq 1 ]; then
        printf '%s... ' "$test_name"
    fi

    run_test
    case $? in
        0)
            # Success
            if [ $VERBOSE -eq 1 ]; then
                printf 'ok\n'
            else
                printf '.'
            fi
            rm -rf $test_log
            ;;

        77)
            # Skip
            if [ $VERBOSE -eq 1 ]; then
                printf 'skipped\n'
            else
                printf 'S'
            fi
            rm -rf $test_log
            ;;

        *)
            # Failure
            if [ $VERBOSE -eq 1 ]; then
                printf 'FAILED\n'
            else
                printf 'F'
            fi

            [ $STOP -eq 1 ] && break
            ;;
    esac
done

if [ $VERBOSE -eq 0 ]; then
    printf '\n'
fi

if [ -n "$(ls -A $suite_log)" ]; then
    for test_log in $suite_log/*; do
        test_name=$(basename $test_log)
        test_path=$suite_srcdir/$test_name
        echo "================================================================="
        echo "$suite_name/$test_name"
        echo "================================================================="
        show_error
        echo
    done
    echo "================================================================="
    exit 1
else
    rm -rf $suite_log
fi
