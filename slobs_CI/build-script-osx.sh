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
-DENABLE_SCRIPTING=false \
-DBUILD_BROWSER=ON \
-DBROWSER_DEPLOY=ON \
-DBUILD_CAPTIONS=ON \
-DBROWSER_FRONTEND_API_SUPPORT=false \
-DBROWSER_PANEL_SUPPORT=false \
-DUSE_UI_LOOP=true \
-DCEF_ROOT_DIR=$PWD/../../cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64 ..

cd ..

cmake --build build --target install --config %BuildConfig% -v

# Install Chromium Embedded Framework
cd packed_build
mkdir Frameworks

cp -R \
../../cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework \
Frameworks/Chromium\ Embedded\ Framework.framework

cp ../../cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libEGL.dylib \
./obs-plugins/libEGL.dylib

cp ../../cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libGLESv2.dylib \
./obs-plugins/libGLESv2.dylib

cp ../../cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libswiftshader_libEGL.dylib \
./obs-plugins/libswiftshader_libEGL.dylib

cp ../../cef_binary_${CEF_MAC_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libswiftshader_libGLESv2.dylib \
./obs-plugins/libswiftshader_libGLESv2.dylib

# Apply new Framework load path
sudo install_name_tool -change \
    @executable_path/../Frameworks/Chromium\ Embedded\ Framework.framework/Chromium\ Embedded\ Framework \
    @rpath/Frameworks/Chromium\ Embedded\ Framework.framework/Chromium\ Embedded\ Framework \
    $PWD/../packed_build/obs-plugins/obs-browser.so

sudo install_name_tool -change \
    @executable_path/../Frameworks/Chromium\ Embedded\ Framework.framework/Chromium\ Embedded\ Framework \
    @rpath/../Frameworks/Chromium\ Embedded\ Framework.framework/Chromium\ Embedded\ Framework \
    $PWD/../packed_build/obs-plugins/obs-browser-page

# Install obs dependencies
cp -R /tmp/obsdeps/bin/. $PWD/../packed_build/bin/

cp -R /tmp/obsdeps/lib/. $PWD/../packed_build/bin/

# Change load path
sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswscale.5.dylib @executable_path/libswscale.5.dylib $PWD/../packed_build/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libjansson.4.dylib @executable_path/libjansson.4.dylib $PWD/../packed_build/bin/libobs.0.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/bin/libavcodec.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/bin/libavcodec.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libavcodec.58.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libavformat.58.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libavutil.56.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libswscale.5.dylib @executable_path/libswscale.5.dylib $PWD/../packed_build/bin/libswscale.5.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libswscale.5.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/bin/libswresample.3.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libswresample.3.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libx264.155.dylib @executable_path/libx264.155.dylib $PWD/../packed_build/obs-plugins/obs-x264.so

sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavdevice.58.dylib @executable_path/libavdevice.58.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/bin/libswscale.5.dylib @executable_path/libswscale.5.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/obs-plugins/obs-ffmpeg.so

sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/bin/obs-ffmpeg-mux
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/obs-ffmpeg-mux
sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/bin/obs-ffmpeg-mux

sudo install_name_tool -change /tmp/obsdeps/bin/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswscale.5.dylib @executable_path/libswscale.5.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libpostproc.55.dylib @executable_path/libpostproc.55.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libavfilter.7.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libpostproc.55.dylib @executable_path/libpostproc.55.dylib $PWD/../packed_build/bin/libpostproc.55.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libpostproc.55.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libavdevice.58.dylib @executable_path/libavdevice.58.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswscale.5.dylib @executable_path/libswscale.5.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libpostproc.55.dylib @executable_path/libpostproc.55.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/bin/libavdevice.58.dylib

cp /usr/local/Cellar/openssl@1.1/1.1.1d/lib/libcrypto.1.1.dylib $PWD/../packed_build/bin/libcrypto.1.1.dylib
cp /usr/local/opt/curl/lib/libcurl.4.dylib $PWD/../packed_build/bin/libcurl.4.dylib
cp /usr/local/Cellar/berkeley-db/18.1.32_1/lib/libdb-18.1.dylib $PWD/../packed_build/bin/libdb-18.1.dylib
cp /usr/local/Cellar/fdk-aac/2.0.1/lib/libfdk-aac.2.dylib $PWD/../packed_build/bin/libfdk-aac.2.dylib
cp /usr/local/opt/freetype/lib/libfreetype.6.dylib $PWD/../packed_build/bin/libfreetype.6.dylib
cp /usr/local/opt/jack/lib/libjack.0.dylib $PWD/../packed_build/bin/libjack.0.dylib
cp /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib $PWD/../packed_build/bin/libmbedcrypto.3.dylib
cp /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib $PWD/../packed_build/bin/libmbedtls.12.dylib
cp /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib $PWD/../packed_build/bin/libmbedx509.0.dylib
cp /usr/local/opt/libpng/lib/libpng16.16.dylib $PWD/../packed_build/bin/libpng16.16.dylib
cp /usr/local/Cellar/speexdsp/1.2.0/lib/libspeexdsp.1.dylib $PWD/../packed_build/bin/libspeexdsp.1.dylib
cp /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib $PWD/../packed_build/bin/libssl.1.1.dylib

chmod u+w $PWD/../packed_build/bin/libspeexdsp.1.dylib
chmod u+w $PWD/../packed_build/bin/libssl.1.1.dylib
chmod u+w $PWD/../packed_build/bin/libfdk-aac.2.dylib
chmod u+w $PWD/../packed_build/bin/libcurl.4.dylib

sudo install_name_tool -change /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib @executable_path/libssl.1.1.dylib $PWD/../packed_build/bin/libdb-18.1.dylib
sudo install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $PWD/../packed_build/bin/libdb-18.1.dylib

sudo install_name_tool -change /usr/local/Cellar/openssl@1.1/1.1.1d/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $PWD/../packed_build/bin/libssl.1.1.dylib

sudo install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @executable_path/libpng16.16.dylib $PWD/../packed_build/bin/libfreetype.6.dylib

sudo install_name_tool -change /usr/local/opt/berkeley-db/lib/libdb-18.1.dylib @executable_path/libdb-18.1.dylib $PWD/../packed_build/bin/libjack.0.dylib

sudo install_name_tool -change /usr/local/Cellar/openssl@1.1/1.1.1d/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $PWD/../packed_build/bin/libcrypto.1.1.dylib

sudo install_name_tool -change libmbedtls.12.dylib @executable_path/libmbedtls.12.dylib $PWD/../packed_build/obs-plugins/obs-outputs.so
sudo install_name_tool -change libmbedcrypto.3.dylib @executable_path/libmbedcrypto.3.dylib $PWD/../packed_build/obs-plugins/obs-outputs.so
sudo install_name_tool -change libmbedx509.0.dylib @executable_path/libmbedx509.0.dylib $PWD/../packed_build/obs-plugins/obs-outputs.so
sudo install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/libcurl.4.dylib $PWD/../packed_build/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/bin/libjansson.4.dylib @executable_path/libjansson.4.dylib $PWD/../packed_build/obs-plugins/obs-outputs.so

sudo install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/libcurl.4.dylib $PWD/../packed_build/obs-plugins/rtmp-services.so
sudo install_name_tool -change /tmp/obsdeps/bin/libjansson.4.dylib @executable_path/libjansson.4.dylib $PWD/../packed_build/obs-plugins/rtmp-services.so

sudo install_name_tool -change /usr/local/opt/freetype/lib/libfreetype.6.dylib @executable_path/libfreetype.6.dylib $PWD/../packed_build/obs-plugins/text-freetype2.so

sudo install_name_tool -change /usr/local/opt/speexdsp/lib/libspeexdsp.1.dylib @executable_path/libspeexdsp.1.dylib $PWD/../packed_build/obs-plugins/obs-filters.so

sudo install_name_tool -change /tmp/obsdeps/bin/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavdevice.58.dylib @executable_path/libavdevice.58.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavutil.56.dylib @executable_path/libavutil.56.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/bin/libswscale.5.dylib @executable_path/libswscale.5.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/bin/libavformat.58.dylib @executable_path/libavformat.58.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/bin/libswresample.3.dylib @executable_path/libswresample.3.dylib $PWD/../packed_build/obs-plugins/slobs-virtual-cam.so


sudo install_name_tool -change /tmp/obsdeps/bin/libmbedtls.12.dylib @executable_path/libmbedtls.12.dylib $PWD/../packed_build/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedx509.0.dylib @executable_path/libmbedx509.0.dylib $PWD/../packed_build/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedcrypto.3.dylib @executable_path/libmbedcrypto.3.dylib $PWD/../packed_build/bin/libavformat.58.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libmbedtls.12.dylib @executable_path/libmbedtls.12.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedx509.0.dylib @executable_path/libmbedx509.0.dylib $PWD/../packed_build/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedcrypto.3.dylib @executable_path/libmbedcrypto.3.dylib $PWD/../packed_build/bin/libavdevice.58.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libmbedtls.12.dylib @executable_path/libmbedtls.12.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedx509.0.dylib @executable_path/libmbedx509.0.dylib $PWD/../packed_build/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedcrypto.3.dylib @executable_path/libmbedcrypto.3.dylib $PWD/../packed_build/bin/libavfilter.7.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libmbedx509.0.dylib @executable_path/libmbedx509.0.dylib $PWD/../packed_build/bin/libmbedtls.12.dylib
sudo install_name_tool -change /tmp/obsdeps/bin/libmbedcrypto.3.dylib @executable_path/libmbedcrypto.3.dylib $PWD/../packed_build/bin/libmbedtls.12.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libmbedcrypto.3.dylib @executable_path/libmbedcrypto.3.dylib $PWD/../packed_build/bin/libmbedx509.0.dylib

sudo install_name_tool -change /tmp/obsdeps/bin/libx264.159.dylib @executable_path/libx264.159.dylib $PWD/../packed_build/obs-plugins/obs-x264.so

sudo install_name_tool -change /tmp/obsdeps/lib/libfreetype.6.dylib @executable_path/libfreetype.6.dylib $PWD/../packed_build/obs-plugins/text-freetype2.so

sudo install_name_tool -change /usr/local/opt/fdk-aac/lib/libfdk-aac.2.dylib @executable_path/libfdk-aac.2.dylib $PWD/../packed_build/obs-plugins/obs-libfdk.so