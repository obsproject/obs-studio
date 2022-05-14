#!/usr/bin/env bash

set -o errexit
set -o pipefail

if [ ${#} -eq 1 -a "${1}" = "VERBOSE" ]; then
    VERBOSITY="-l debug"
else
    VERBOSITY=""
fi

if [ "${CI}" ]; then
    MODE="--check"
else
    MODE="-i"
fi

# Runs the formatter in parallel on the code base.
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

# Get CPU count
OS=$(uname)
NPROC=1
if [[ ${OS} = "Linux" ]] ; then
    NPROC=$(nproc)
elif [[ ${OS} = "Darwin" ]] ; then
    NPROC=$(sysctl -n hw.physicalcpu)
fi

# Discover clang-format
if ! type cmake-format 2> /dev/null ; then
    echo "Required cmake-format not found"
    exit 1
fi

find . -type d \( \
    -path ./\*build -o \
    -path ./deps/jansson -o \
    -path ./plugins/decklink/\*/decklink-sdk -o \
    -path ./plugins/enc-amf -o \
    -path ./plugins/mac-syphon/syphon-framework -o \
    -path ./plugins/obs-outputs/ftl-sdk -o \
    -path ./plugins/obs-vst -o \
    -path ./plugins/obs-browser -o \
    -path ./plugins/win-dshow/libdshowcapture -o \
    -path ./plugins/obs-websocket/deps \
\) -prune -false -type f -o \
    -name 'CMakeLists.txt' -or \
    -name '*.cmake' \
 | xargs -L10 -P ${NPROC} cmake-format ${MODE} ${VERBOSITY}
