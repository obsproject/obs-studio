#!/bin/sh

while [ -n "$1" ]; do
    suite=$1
    if [ -x $top_srcdir/test/suites/$suite/run ]; then
        SUITES="$SUITES $suite"
    else
        echo "No such suite: $suite"
        exit 1
    fi
    shift
done

if [ -z "$SUITES" ]; then
    suitedirs=$top_srcdir/test/suites/*
    for suitedir in $suitedirs; do
        if [ -d $suitedir ]; then
            SUITES="$SUITES `basename $suitedir`"
        fi
    done
fi

[ -z "$STOP" ] && STOP=0

suites_srcdir=$top_srcdir/test/suites
suites_builddir=suites
scriptdir=$top_srcdir/test/scripts
logdir=logs
bindir=bin
export suites_srcdir suites_builddir scriptdir logdir bindir

passed=0
failed=0
for suite in $SUITES; do
    echo "Suite: $suite"
    if $suites_srcdir/$suite/run $suite; then
        passed=`expr $passed + 1`
    else
        failed=`expr $failed + 1`
        [ $STOP -eq 1 ] && break
    fi
done

if [ $failed -gt 0 ]; then
    echo "$failed of `expr $passed + $failed` test suites failed"
    exit 1
else
    echo "$passed test suites passed"
    rm -rf $logdir
fi
