#!/bin/sh
set -ex

export LD_LIBRARY_PATH=/opt/qt59/lib/x86_64-linux-gnu:/opt/qt59/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/opt/qt59/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=/opt/qt59/bin:$PATH

ccache -s || echo "CCache is not available."
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
