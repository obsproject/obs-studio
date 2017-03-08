#!/bin/sh
set -ex

ccache -s || echo "CCache is not available."
mkdir build && cd build
cmake ..
