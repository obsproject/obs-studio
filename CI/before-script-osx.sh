# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH

git fetch --tags

mkdir build
cd build
cmake -DENABLE_SPARKLE_UPDATER=ON \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
-DDISABLE_PYTHON=ON \
-DQTDIR=/usr/local/Cellar/qt/5.14.1 \
-DDepsPath=/tmp/obsdeps \
-DVLCPath=$PWD/../../vlc-3.0.8 \
-DBUILD_BROWSER=ON \
-DBROWSER_DEPLOY=ON \
-DWITH_RTMPS=ON \
-DCEF_ROOT_DIR=$PWD/../../cef_binary_${CEF_BUILD_VERSION}_macosx64 ..
