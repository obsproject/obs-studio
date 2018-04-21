#!/usr/bin/env bash

exists()
{
  command -v "$1" >/dev/null 2>&1
}

if ! exists nasm; then
    echo "nasm not found. Try brew install nasm"
    exit
fi

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
mkdir $DEPS_DEST/lib

# OSX COMPAT
export MACOSX_DEPLOYMENT_TARGET=10.9

# If you need an olders SDK and Xcode won't give it to you
# https://github.com/phracker/MacOSX-SDKs

# libopus
curl -L -O http://downloads.xiph.org/releases/opus/opus-1.1.3.tar.gz
tar -xf opus-1.1.3.tar.gz
cd ./opus-1.1.3
mkdir build
cd ./build
../configure --disable-shared --enable-static --prefix="/tmp/obsdeps"
make -j 12
make install

cd $WORK_DIR

# libogg
curl -L -O http://downloads.xiph.org/releases/ogg/libogg-1.3.2.tar.gz
tar -xf libogg-1.3.2.tar.gz
cd ./libogg-1.3.2
mkdir build
cd ./build
../configure --disable-shared --enable-static --prefix="/tmp/obsdeps"
make -j 12
make install

cd $WORK_DIR

# libvorbis
curl -L -O http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.tar.gz
tar -xf libvorbis-1.3.5.tar.gz
cd ./libvorbis-1.3.5
mkdir build
cd ./build
../configure --disable-shared --enable-static --prefix="/tmp/obsdeps"
make -j 12
make install

cd $WORK_DIR

# libvpx
curl -L -O http://storage.googleapis.com/downloads.webmproject.org/releases/webm/libvpx-1.6.0.tar.bz2
tar -xf libvpx-1.6.0.tar.bz2
cd ./libvpx-1.6.0
mkdir build
cd ./build
../configure --disable-shared --libdir="/tmp/obsdeps/bin"
make -j 12
make install

cd $WORK_DIR

# x264
git clone git://git.videolan.org/x264.git
cd ./x264
git checkout origin/stable
mkdir build
cd ./build
../configure --extra-ldflags="-mmacosx-version-min=10.9" --enable-static --prefix="/tmp/obsdeps"
make -j 12
make install
../configure --extra-ldflags="-mmacosx-version-min=10.9" --enable-shared --libdir="/tmp/obsdeps/bin" --prefix="/tmp/obsdeps"
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

export LDFLAGS="-L/tmp/obsdeps/lib"
export CFLAGS="-I/tmp/obsdeps/include"

# FFMPEG
curl -L -O https://github.com/FFmpeg/FFmpeg/archive/n3.2.2.zip
unzip ./n3.2.2.zip
cd ./FFmpeg-n3.2.2
mkdir build
cd ./build
../configure --extra-ldflags="-mmacosx-version-min=10.9" --enable-shared --disable-static --shlibdir="/tmp/obsdeps/bin" --enable-gpl --disable-doc --enable-libx264 --enable-libopus --enable-libvorbis --enable-libvpx --disable-outdev=sdl
make -j 12
find . -name \*.dylib -exec cp \{\} $DEPS_DEST/bin/ \;
rsync -avh --include="*/" --include="*.h" --exclude="*" ../* $DEPS_DEST/include/
rsync -avh --include="*/" --include="*.h" --exclude="*" ./* $DEPS_DEST/include/

#luajit
curl -L -O https://luajit.org/download/LuaJIT-2.0.5.tar.gz
tar -xf LuaJIT-2.0.5.tar.gz
cd LuaJIT-2.0.5
make PREFIX=/tmp/obsdeps
make PREFIX=/tmp/obsdeps install
find /tmp/obsdeps/lib -name libluajit\*.dylib -exec cp \{\} $DEPS_DEST/lib/ \;
rsync -avh --include="*/" --include="*.h" --exclude="*" src/* $DEPS_DEST/include/
make PREFIX=/tmp/obsdeps uninstall

cd $WORK_DIR

tar -czf osx-deps.tar.gz obsdeps

cp ./osx-deps.tar.gz $CURDIR