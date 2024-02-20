#!/usr/bin/env bash

##############################################################################
# FreeBSD support functions
##############################################################################
#
# This script file can be included in build scripts for FreeBSD.
#
##############################################################################

# Setup build environment

if [ "${TERM-}" -a -x /usr/local/bin/tput ]; then
    COLOR_RED=$(/usr/local/bin/tput setaf 1)
    COLOR_GREEN=$(/usr/local/bin/tput setaf 2)
    COLOR_BLUE=$(/usr/local/bin/tput setaf 4)
    COLOR_ORANGE=$(/usr/local/bin/tput setaf 3)
    COLOR_RESET=$(/usr/local/bin/tput sgr0)
else
    COLOR_RED=""
    COLOR_GREEN=""
    COLOR_BLUE=""
    COLOR_ORANGE=""
    COLOR_RESET=""
fi

if [ "${CI}" -o "${QUIET}" ]; then
    export CURLCMD="curl --silent --show-error --location -O"
else
    export CURLCMD="curl --progress-bar --location --continue-at - -O"
fi

_add_ccache_to_path() {
    if [ "${CMAKE_CCACHE_OPTIONS}" ]; then
        PATH="/usr/local/opt/ccache/libexec:${PATH}"
        status "Compiler Info:"
        local IFS=$'\n'
        for COMPILER_INFO in $(type cc c++ gcc g++ clang clang++ || true); do
            info "${COMPILER_INFO}"
        done
    fi
}
