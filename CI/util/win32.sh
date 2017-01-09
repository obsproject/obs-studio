#/bin/bash

cd x264
make clean
LDFLAGS="-static-libgcc" ./configure --enable-shared --enable-win32thread --disable-avs --disable-ffms --disable-gpac --disable-interlaced --disable-lavf --cross-prefix=i686-w64-mingw32- --host=i686-pc-mingw32 --prefix="/home/jim/packages/win32"
make -j6 fprofiled VIDS="CITY_704x576_60_orig_01.yuv"
make install
i686-w64-mingw32-dlltool -z /home/jim/packages/win32/bin/x264.orig.def --export-all-symbols /home/jim/packages/win32/bin/libx264-148.dll
grep "EXPORTS\|x264" /home/jim/packages/win32/bin/x264.orig.def > /home/jim/packages/win32/bin/x264.def
rm -f /home/jim/packages/win32/bin/x264.org.def
sed -i -e "/\\t.*DATA/d" -e "/\\t\".*/d" -e "s/\s@.*//" /home/jim/packages/win32/bin/x264.def
i686-w64-mingw32-dlltool -m i386 -d /home/jim/packages/win32/bin/x264.def -l /home/jim/packages/win32/bin/x264.lib -D /home/jim/win32/packages/bin/libx264-148.dll
cd ..

cd opus
make clean
LDFLAGS="-static-libgcc" ./configure -host=i686-w64-mingw32 --prefix="/home/jim/packages/win32" --enable-shared
make -j6
make install
cd ..

cd zlib/build32
make clean
cmake .. -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_INSTALL_PREFIX=/home/jim/packages/win32 -DINSTALL_PKGCONFIG_DIR=/home/jim/packages/win32/lib/pkgconfig -DCMAKE_RC_COMPILER=i686-w64-mingw32-windres -DCMAKE_SHARED_LINKER_FLAGS="-static-libgcc"
make -j6
make install
mv ../../win32/lib/libzlib.dll.a ../../win32/lib/libz.dll.a
mv ../../win32/lib/libzlibstatic.a ../../win32/lib/libz.a
cp ../win32/zlib.def /home/jim/packages/win32/bin
i686-w64-mingw32-dlltool -m i386 -d ../win32/zlib.def -l /home/jim/packages/win32/bin/zlib.lib -D /home/jim/win32/packages/bin/zlib.dll
cd ../..

cd libpng
make clean
PKG_CONFIG_PATH="/home/jim/packages/win32/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win32/lib -static-libgcc" CPPFLAGS="-I/home/jim/packages/win32/include" ./configure -host=i686-w64-mingw32 --prefix="/home/jim/packages/win32" --enable-shared
make -j6
make install
cd ..

cd libogg
make clean
PKG_CONFIG_PATH="/home/jim/packages/win32/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win32/lib -static-libgcc" CPPFLAGS="-I/home/jim/packages/win32/include" ./configure -host=i686-w64-mingw32 --prefix="/home/jim/packages/win32" --enable-shared
make -j6
make install
cd ..

cd libvorbis
make clean
PKG_CONFIG_PATH="/home/jim/packages/win32/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win32/lib -static-libgcc" CPPFLAGS="-I/home/jim/packages/win32/include" ./configure -host=i686-w64-mingw32 --prefix="/home/jim/packages/win32" --enable-shared --with-ogg="/home/jim/packages/win32"
make -j6
make install
cd ..

cd libvpxbuild
make clean
PKG_CONFIG_PATH="/home/jim/packages/win32/lib/pkgconfig" CROSS=i686-w64-mingw32- LDFLAGS="-static-libgcc" ../libvpx/configure --prefix=/home/jim/packages/win32 --enable-vp8 --enable-vp9 --disable-docs --disable-examples --enable-shared --disable-static --enable-runtime-cpu-detect --enable-realtime-only --disable-install-bins --disable-install-docs --disable-unit-tests --target=x86-win32-gcc
make -j6
make install
i686-w64-mingw32-dlltool -m i386 -d libvpx.def -l /home/jim/packages/win32/bin/vpx.lib -D /home/jim/win32/packages/bin/libvpx-1.dll
cd ..

cd ffmpeg
make clean
cp /media/sf_linux/nvEncodeAPI.h /home/jim/packages/win32/include
PKG_CONFIG_PATH="/home/jim/packages/win32/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win32/lib -static-libgcc" CFLAGS="-I/home/jim/packages/win32/include" ./configure --enable-memalign-hack --enable-gpl --disable-programs --disable-doc --arch=x86 --enable-shared --enable-nvenc --enable-libx264 --enable-libopus --enable-libvorbis --enable-libvpx --disable-debug --cross-prefix=i686-w64-mingw32- --target-os=mingw32 --pkg-config=pkg-config --prefix="/home/jim/packages/win32" --disable-postproc
read -n1 -r -p "Press any key to continue building FFmpeg..." key
make -j6
make install
cd ..
