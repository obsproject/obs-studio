#!/usr/bin/env bash

CURDIR=$(pwd)

# the temp directory
WORK_DIR=`mktemp -d`

# deletes the temp directory
function cleanup {
  #rm -rf "$WORK_DIR"
  echo "Deleted temp working directory $WORK_DIR"
}

# register the cleanup function to be called on the EXIT signal
trap cleanup EXIT

cd $WORK_DIR

DEPS_DEST=$WORK_DIR/obsdeps

# make dest dirs
mkdir $DEPS_DEST
mkdir $DEPS_DEST/bin
mkdir $DEPS_DEST/include

# OSX COMPAT
export MACOSX_DEPLOYMENT_TARGET=10.9

# If you need an olders SDK and Xcode won't give it to you
# https://github.com/phracker/MacOSX-SDKs

# x264
git clone git://git.videolan.org/x264.git
cd ./x264
mkdir build
cd ./build
../configure --extra-ldflags="-mmacosx-version-min=10.9" --enable-shared --libdir="/tmp/obsdeps/bin"
make -j 12
ln -f -s libx264.*.dylib libx264.dylib
find . -name \*.dylib -exec cp \{\} $DEPS_DEST/bin/ \;
rsync -avh --include="*/" --include="*.h" --exclude="*" ../* $DEPS_DEST/include/
rsync -avh --include="*/" --include="*.h" --exclude="*" ./* $DEPS_DEST/include/

cd $WORK_DIR

# janson
curl -L -O http://www.digip.org/jansson/releases/jansson-2.9.tar.gz
tar -xf jansson-2.9.tar.gz
cd jansson-2.9
mkdir build
cd ./build
../configure --libdir="/tmp/obsdeps/bin" --enable-shared --disable-static
make -j 12
find . -name \*.dylib -exec cp \{\} $DEPS_DEST/bin/ \;
rsync -avh --include="*/" --include="*.h" --exclude="*" ../* $DEPS_DEST/include/
rsync -avh --include="*/" --include="*.h" --exclude="*" ./* $DEPS_DEST/include/

cd $WORK_DIR

# FFMPEG
curl -L -O https://github.com/FFmpeg/FFmpeg/archive/n3.2.2.zip
unzip ./n3.2.2.zip
cd ./FFmpeg-n3.2.2
mkdir build
cd ./build
../configure --extra-ldflags="-mmacosx-version-min=10.9" --enable-shared --disable-static --shlibdir="/tmp/obsdeps/bin"
make -j 12
find . -name \*.dylib -exec cp \{\} $DEPS_DEST/bin/ \;
rsync -avh --include="*/" --include="*.h" --exclude="*" ../* $DEPS_DEST/include/
rsync -avh --include="*/" --include="*.h" --exclude="*" ./* $DEPS_DEST/include/

cd $WORK_DIR

tar -czf osx-deps.tar.gz obsdeps

cp ./osx-deps.tar.gz $CURDIR