#!/usr/bin/env bash
# Original source https://github.com/Project-OSRM/osrm-backend/blob/master/scripts/format.sh

set +x
set -o errexit
set -o pipefail
set -o nounset

# Runs the Clang Formatter in parallel on the code base.
# Return codes:
#  - 1 there are files to be formatted
#  - 0 everything looks fine

# Get CPU count
OS=$(uname)
NPROC=1
if [[ $OS = "Linux" ]] ; then
    NPROC=$(nproc)
elif [[ ${OS} = "Darwin" ]] ; then
    NPROC=$(sysctl -n hw.physicalcpu)
fi

# Discover clang-format
if type clang-format-8 2> /dev/null ; then
    CLANG_FORMAT=clang-format-8
else
    CLANG_FORMAT=clang-format
fi

find . -type d \( -path ./deps -o -path ./cmake -o -path ./plugins/decklink/win -o -path ./plugins/decklink/mac -o -path ./plugins/decklink/linux -o -path ./build \) -prune -type f -o -name '*.h' -or -name '*.hpp' -or -name '*.m' -or -name '*.mm' -or -name '*.c' -or -name '*.cpp' \
| xargs -I{} -P ${NPROC} ${CLANG_FORMAT} -i -style=file  -fallback-style=none {}
