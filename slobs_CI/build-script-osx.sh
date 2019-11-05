export PATH=/usr/local/opt/ccache/libexec:$PATH

git fetch --tags

mkdir build packed_build

cd build
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 \
-DDepsPath=/tmp/obsdeps \
-DCMAKE_INSTALL_PREFIX=$PWD/../packed_build \
-DVLCPath=$PWD/../../vlc-3.0.4 \
-DENABLE_UI=false \
-DDISABLE_UI=true \
-DCOPIED_DEPENDENCIES=false \
-DCOPY_DEPENDENCIES=true \
-DENABLE_SCRIPTING=false ..

cd ..

cmake --build build --target install --config Debug

