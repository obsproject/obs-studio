#!/bin/bash

./formatcode.sh
if ! ./CI/check-format.sh; then
	exit 1
fi

set -ex
ccache -s || echo "CCache is not available."
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="/usr" -DBUILD_CAPTIONS=ON -DBUILD_BROWSER=ON -DCEF_ROOT_DIR="../cef_binary_${CEF_BUILD_VERSION}_linux64" ..
make -j4
sudo make DESTDIR=appdir -j4 install
