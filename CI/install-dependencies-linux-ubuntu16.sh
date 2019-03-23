#!/bin/sh
set -ex

sudo apt-get -qq update
sudo apt-get install -y \
        build-essential \
        checkinstall \
        cmake \
        libasound2-dev \
        libavcodec-dev \
        libavdevice-dev \
        libavfilter-dev \
        libavformat-dev \
        libavutil-dev \
        libcurl4-openssl-dev \
        libfdk-aac-dev \
        libfontconfig-dev \
        libfreetype6-dev \
        libgl1-mesa-dev \
        libjack-jackd2-dev \
        libjansson-dev \
        libluajit-5.1-dev \
        libpulse-dev \
        libqt5x11extras5-dev \
        libspeexdsp-dev \
        libswresample-dev \
        libswscale-dev \
        libudev-dev \
        libv4l-dev \
        libvlc-dev \
        libx11-dev \
        libx264-dev \
        libxcb-shm0-dev \
        libxcb-xinerama0-dev \
        libxcomposite-dev \
        libxinerama-dev \
        pkg-config \
        python3-dev \
        qtbase5-dev \
        libqt5svg5-dev \
        swig


# build mbedTLS
cd ~/projects
mkdir mbedtls
cd mbedtls
mbedtlsPath=$PWD
curl -L -O https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.12.0.tar.gz
tar -xf mbedtls-2.12.0.tar.gz
mkdir build
cd ./build
cmake -DENABLE_TESTING=Off -DUSE_SHARED_MBEDTLS_LIBRARY=On ../mbedtls-mbedtls-2.12.0
make -j 12
sudo make install

# build cef
cd ~/projects
curl -L "http://opensource.spotify.com/cefbuilds/cef_binary_73.1.12%2Bgee4b49f%2Bchromium-73.0.3683.75_linux64_minimal.tar.bz2" -o "cef.tar.bz2"
tar -xjf "cef.tar.bz2" --one-top-level=cef --strip-components=1
cd cef && mkdir build
cd build
cmake -DCMAKE_CXX_FLAGS="-Wno-attributes" ..
make -j4

# return to OBS build dir
cd $APPVEYOR_BUILD_FOLDER
