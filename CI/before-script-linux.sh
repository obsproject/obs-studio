#!/bin/bash

./formatcode.sh
if ! ./CI/check-format.sh; then
	exit 1
fi

set -ex
ccache -s || echo "CCache is not available."
mkdir build && cd build
cmake -DBUILD_CAPTIONS=ON ..
