# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH

mkdir build
cd build
cmake -DENABLE_SPARKLE_UPDATER=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DDepsPath=/tmp/obsdeps -DVLCPath=$PWD/../../vlc-master -DBUILD_BROWSER=ON -DCEF_ROOT_DIR=$PWD/../../cef_binary_${CEF_BUILD_VERSION}_macosx64 ..
