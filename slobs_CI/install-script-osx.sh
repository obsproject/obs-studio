echo "RUNNING INSTALL SCRIPT"

# Echo all commands before executing
set -v

cd ../

brew update

brew install ccache mbedtls ffmpeg x264 cmake p7zip curl berkeley-db fdk-aac speexdsp python

export PATH=/usr/local/opt/ccache/libexec:$PATH
ccache -s || echo "CCache is not available."

# Fetch and untar prebuilt OBS deps that are compatible with older versions of OSX
hr "Downloading OBS deps"
wget --quiet --retry-connrefused --waitretry=1 https://obs-studio-deployment.s3-us-west-2.amazonaws.com/osx-deps-2020-04-24.tar.gz
tar -xf ./osx-deps-2020-04-24.tar.gz -C /tmp

# Fetch vlc codebase
hr "Downloading VLC repo"
wget --quiet --retry-connrefused --waitretry=1 https://downloads.videolan.org/vlc/3.0.4/vlc-3.0.4.tar.xz
tar -xf vlc-3.0.4.tar.xz

# CEF Stuff
hr "Downloading CEF"
wget --quiet --retry-connrefused --waitretry=1 https://obs-studio-deployment.s3-us-west-2.amazonaws.com/cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64.tar.bz2
tar -xf ./cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64.tar.bz2
cd ./cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64
# remove a broken test
sed -i '.orig' '/add_subdirectory(tests\/ceftests)/d' ./CMakeLists.txt
# target 10.11
sed -i '.orig' s/\"10.9\"/\"10.11\"/ ./cmake/cef_variables.cmake
mkdir build
cd ./build
cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 ..
make -j4
mkdir libcef_dll
cd ../../