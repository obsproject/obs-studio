# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH

mkdir build
cd build
cmake -DPYTHON_LIBRARY=/usr/local/Cellar/python3/3.6.4/Frameworks/Python.framework/Versions/3.6/lib/libpython3.6.dylib -DPYTHON_INCLUDE_DIR=/usr/local/Cellar/python3/3.6.4/Frameworks/Python.framework/Versions/3.6/include/python3.6m -DENABLE_SPARKLE_UPDATER=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 -DDepsPath=/tmp/obsdeps -DVLCPath=$PWD/../../vlc-master -DBUILD_BROWSER=ON -DCEF_ROOT_DIR=$PWD/../../cef_binary_${CEF_BUILD_VERSION}_macosx64 ..
