mkdir build
cd build
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DDepsPath=/tmp/obsdeps -DVLCPath=$PWD/../../vlc-master -DBUILD_BROWSER=ON -DCEF_ROOT_DIR=$PWD/../../cef_binary_3.2987.1588.g1952835_macosx64 ..
