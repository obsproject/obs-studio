#!/bin/sh
set -ex

ccache -s || echo "CCache is not available."
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
