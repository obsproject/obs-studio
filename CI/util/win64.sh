#/bin/bash

cd x264
make clean
LDFLAGS="-static-libgcc" ./configure --enable-shared --enable-win32thread --disable-avs --disable-ffms --disable-gpac --disable-interlaced --disable-lavf --cross-prefix=x86_64-w64-mingw32- --host=x86_64-pc-mingw32 --prefix="/home/jim/packages/win64"
make -j6 fprofiled VIDS="CITY_704x576_60_orig_01.yuv"
make install
x86_64-w64-mingw32-dlltool -z /home/jim/packages/win64/bin/x264.orig.def --export-all-symbols /home/jim/packages/win64/bin/libx264-148.dll
grep "EXPORTS\|x264" /home/jim/packages/win64/bin/x264.orig.def > /home/jim/packages/win64/bin/x264.def
rm -f /home/jim/packages/win64/bin/x264.org.def
sed -i -e "/\\t.*DATA/d" -e "/\\t\".*/d" -e "s/\s@.*//" /home/jim/packages/win64/bin/x264.def
x86_64-w64-mingw32-dlltool -m i386:x86-64 -d /home/jim/packages/win64/bin/x264.def -l /home/jim/packages/win64/bin/x264.lib -D /home/jim/win64/packages/bin/libx264-148.dll
cd ..

cd opus
make clean
LDFLAGS="-static-libgcc" ./configure -host=x86_64-w64-mingw32 --prefix="/home/jim/packages/win64" --enable-shared
make -j6
make install
cd ..

cd zlib/build64
make clean
cmake .. -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_INSTALL_PREFIX=/home/jim/packages/win64 -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DCMAKE_SHARED_LINKER_FLAGS="-static-libgcc"
make -j6
make install
mv ../../win64/lib/libzlib.dll.a ../../win64/lib/libz.dll.a
mv ../../win64/lib/libzlibstatic.a ../../win64/lib/libz.a
cp ../win64/zlib.def /home/jim/packages/win64/bin
x86_64-w64-mingw32-dlltool -m i386:x86-64 -d ../win32/zlib.def -l /home/jim/packages/win64/bin/zlib.lib -D /home/jim/win64/packages/bin/zlib.dll
cd ../..

cd libpng
make clean
PKG_CONFIG_PATH="/home/jim/packages/win64/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win64/lib" CPPFLAGS="-I/home/jim/packages/win64/include" ./configure -host=x86_64-w64-mingw32 --prefix="/home/jim/packages/win64" --enable-shared
make -j6
make install
cd ..

cd libogg
make clean
PKG_CONFIG_PATH="/home/jim/packages/win64/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win64/lib -static-libgcc" CPPFLAGS="-I/home/jim/packages/win64/include" ./configure -host=x86_64-w64-mingw32 --prefix="/home/jim/packages/win64" --enable-shared
make -j6
make install
cd ..

cd libvorbis
make clean
PKG_CONFIG_PATH="/home/jim/packages/win64/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win64/lib -static-libgcc" CPPFLAGS="-I/home/jim/packages/win64/include" ./configure -host=x86_64-w64-mingw32 --prefix="/home/jim/packages/win64" --enable-shared --with-ogg="/home/jim/packages/win64"
make -j6
make install
cd ..

cd libvpxbuild
make clean
PKG_CONFIG_PATH="/home/jim/packages/win64/lib/pkgconfig" CROSS=x86_64-w64-mingw32- LDFLAGS="-static-libgcc" ../libvpx/configure --prefix=/home/jim/packages/win64 --enable-vp8 --enable-vp9 --disable-docs --disable-examples --enable-shared --disable-static --enable-runtime-cpu-detect --enable-realtime-only --disable-install-bins --disable-install-docs --disable-unit-tests --target=x86_64-win64-gcc
make -j6
make install
x86_64-w64-mingw32-dlltool -m i386:x86-64 -d libvpx.def -l /home/jim/packages/win64/bin/vpx.lib -D /home/jim/win64/packages/bin/libvpx-1.dll
cd ..

cd ffmpeg
make clean
cp /media/sf_linux/nvEncodeAPI.h /home/jim/packages/win64/include
PKG_CONFIG_PATH="/home/jim/packages/win64/lib/pkgconfig" LDFLAGS="-L/home/jim/packages/win64/lib" CPPFLAGS="-I/home/jim/packages/win64/include" ./configure --enable-memalign-hack --enable-gpl --disable-doc --arch=x86_64 --enable-shared --enable-nvenc --enable-libx264 --enable-libopus --enable-libvorbis --enable-libvpx --disable-debug --cross-prefix=x86_64-w64-mingw32- --target-os=mingw32 --pkg-config=pkg-config --prefix="/home/jim/packages/win64" --disable-postproc
read -n1 -r -p "Press any key to continue building FFmpeg..." key
make -j6
make install
cd ..
