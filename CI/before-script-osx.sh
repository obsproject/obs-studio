mkdir build
cd build
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DDepsPath=/tmp/obsdeps -DBUILD_BROWSER=ON -DCEF_ROOT_DIR=$PWD/../../cef_binary_3.2704.1434.gec3e9ed_macosx64 ..