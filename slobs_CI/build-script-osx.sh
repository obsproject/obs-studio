export PATH=/usr/local/opt/ccache/libexec:$PATH
set -e
set -v

DEPS_DIR=$PWD/build/deps
mkdir packed_build
PACKED_BUILD=$PWD/packed_build
cd build
echo $DEPS_DIR
echo $PWD 
echo $PACKED_BUILD

cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
-DDepsPath=$DEPS_DIR/obsdeps \
-DCMAKE_INSTALL_PREFIX=$PACKED_BUILD \
-DVLCPath=$DEPS_DIR/vlc-$VLC_VERSION \
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
-DCHECK_FOR_SERVICE_UPDATES=true \
-DCEF_ROOT_DIR=$DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64 \
-DDISABLE_LIBFDK=true ..

cd ..

cmake --build build --target install --config %BuildConfig% -v

# Install Chromium Embedded Framework
cd $PACKED_BUILD
mkdir Frameworks

cp -R \
$DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework \
Frameworks/Chromium\ Embedded\ Framework.framework

cp $DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libEGL.dylib \
./obs-plugins/libEGL.dylib

cp $DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libGLESv2.dylib \
./obs-plugins/libGLESv2.dylib

cp $DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libswiftshader_libEGL.dylib \
./obs-plugins/libswiftshader_libEGL.dylib

cp $DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libswiftshader_libGLESv2.dylib \
./obs-plugins/libswiftshader_libGLESv2.dylib

cp $DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework/Libraries/libvk_swiftshader.dylib \
./obs-plugins/libvk_swiftshader.dylib

cp $DEPS_DIR/cef_binary_${CEF_MAC_BUILD_VERSION}_macos_x86_64/Release/Chromium\ Embedded\ Framework.framework/Libraries/vk_swiftshader_icd.json \
./obs-plugins/vk_swiftshader_icd.json

if ! [ "${CEF_MAC_BUILD_VERSION}" -le 3770 ]; then
    cp -R "../build/plugins/obs-browser/obs64 Helper.app" "./Frameworks/obs64 Helper.app"
    cp -R "../build/plugins/obs-browser/obs64 Helper (GPU).app" "./Frameworks/obs64 Helper (GPU).app"
    cp -R "../build/plugins/obs-browser/obs64 Helper (Plugin).app" "./Frameworks/obs64 Helper (Plugin).app"
    cp -R "../build/plugins/obs-browser/obs64 Helper (Renderer).app" "./Frameworks/obs64 Helper (Renderer).app"
fi

# Apply new Framework load path
sudo install_name_tool -change \
    @executable_path/../Frameworks/Chromium\ Embedded\ Framework.framework/Chromium\ Embedded\ Framework \
    @rpath/Frameworks/Chromium\ Embedded\ Framework.framework/Chromium\ Embedded\ Framework \
    $PACKED_BUILD/obs-plugins/obs-browser.so

# Install obs dependencies
cp -R $DEPS_DIR/obsdeps/lib/. $PACKED_BUILD/bin/

# Change load path
sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswscale.5.dylib @executable_path/libswscale.5.dylib $PACKED_BUILD/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/bin/libobs.0.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libjansson.4.dylib @executable_path/libjansson.4.dylib $PACKED_BUILD/bin/libobs.0.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/bin/libavcodec.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/bin/libavcodec.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libavcodec.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libx264.161.dylib @executable_path/libx264.161.dylib $PACKED_BUILD/bin/libavcodec.58.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libx264.161.dylib @executable_path/libx264.161.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.2.24.0.dylib @executable_path/libmbedtls.2.24.0.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.2.24.0.dylib @executable_path/libmbedx509.2.24.0.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.2.24.0.dylib @executable_path/libmbedcrypto.2.24.0.dylib $PACKED_BUILD/bin/libavformat.58.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libavutil.56.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libswscale.5.dylib @executable_path/libswscale.5.dylib $PACKED_BUILD/bin/libswscale.5.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libswscale.5.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/bin/libswresample.3.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libswresample.3.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libx264.155.dylib @executable_path/libx264.155.dylib $PACKED_BUILD/obs-plugins/obs-x264.so

sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavdevice.58.dylib @executable_path/libavdevice.58.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/lib/libswscale.5.dylib @executable_path/libswscale.5.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/obs-plugins/obs-ffmpeg.so

sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/bin/obs-ffmpeg-mux
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/obs-ffmpeg-mux
sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/bin/obs-ffmpeg-mux

sudo install_name_tool -change /tmp/obsdeps/lib/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswscale.5.dylib @executable_path/libswscale.5.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libpostproc.55.dylib @executable_path/libpostproc.55.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libx264.161.dylib @executable_path/libx264.161.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.2.24.0.dylib @executable_path/libmbedtls.2.24.0.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.2.24.0.dylib @executable_path/libmbedx509.2.24.0.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.2.24.0.dylib @executable_path/libmbedcrypto.2.24.0.dylib $PACKED_BUILD/bin/libavfilter.7.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libpostproc.55.dylib @executable_path/libpostproc.55.dylib $PACKED_BUILD/bin/libpostproc.55.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libpostproc.55.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libavdevice.58.dylib @executable_path/libavdevice.58.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswscale.5.dylib @executable_path/libswscale.5.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libpostproc.55.dylib @executable_path/libpostproc.55.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libx264.161.dylib @executable_path/libx264.161.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.2.24.0.dylib @executable_path/libmbedtls.2.24.0.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.2.24.0.dylib @executable_path/libmbedx509.2.24.0.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.2.24.0.dylib @executable_path/libmbedcrypto.2.24.0.dylib $PACKED_BUILD/bin/libavdevice.58.dylib

cp /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib $PACKED_BUILD/bin/libcrypto.1.1.dylib
cp /usr/local/opt/curl/lib/libcurl.4.dylib $PACKED_BUILD/bin/libcurl.4.dylib
cp /usr/local/opt/berkeley-db/lib/libdb-18.1.dylib $PACKED_BUILD/bin/libdb-18.1.dylib
cp /usr/local/opt/freetype/lib/libfreetype.6.dylib $PACKED_BUILD/bin/libfreetype.6.dylib
cp /usr/local/opt/jack/lib/libjack.0.dylib $PACKED_BUILD/bin/libjack.0.dylib
cp /usr/local/opt/libpng/lib/libpng16.16.dylib $PACKED_BUILD/bin/libpng16.16.dylib
cp /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib $PACKED_BUILD/bin/libssl.1.1.dylib
cp /usr/local/opt/xz/lib/liblzma.5.dylib $PACKED_BUILD/bin/liblzma.5.dylib

chmod u+w $PACKED_BUILD/bin/lib*.dylib

sudo install_name_tool -change /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib @executable_path/libssl.1.1.dylib $PACKED_BUILD/bin/libdb-18.1.dylib
sudo install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $PACKED_BUILD/bin/libdb-18.1.dylib

sudo install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $PACKED_BUILD/bin/libssl.1.1.dylib

sudo install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @executable_path/libpng16.16.dylib $PACKED_BUILD/bin/libfreetype.6.dylib

sudo install_name_tool -change /usr/local/opt/berkeley-db/lib/libdb-18.1.dylib @executable_path/libdb-18.1.dylib $PACKED_BUILD/bin/libjack.0.dylib

sudo install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib $PACKED_BUILD/bin/libcrypto.1.1.dylib

sudo install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/libcurl.4.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libjansson.4.dylib @executable_path/libjansson.4.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.13.dylib @executable_path/libmbedtls.13.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.2.24.0.dylib @executable_path/libmbedtls.2.24.0.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.1.dylib @executable_path/libmbedx509.1.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.5.dylib @executable_path/libmbedcrypto.5.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.2.24.0.dylib  @executable_path/libmbedcrypto.2.24.0.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.2.24.0.dylib @executable_path/libmbedx509.2.24.0.dylib $PACKED_BUILD/obs-plugins/obs-outputs.so

sudo install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/libcurl.4.dylib $PACKED_BUILD/obs-plugins/rtmp-services.so
sudo install_name_tool -change /tmp/obsdeps/lib/libjansson.4.dylib @executable_path/libjansson.4.dylib $PACKED_BUILD/obs-plugins/rtmp-services.so

sudo install_name_tool -change /tmp/obsdeps/lib/libfreetype.6.dylib @executable_path/libfreetype.6.dylib $PACKED_BUILD/obs-plugins/text-freetype2.so

sudo install_name_tool -change /tmp/obsdeps/lib/libspeexdsp.1.dylib  @executable_path/libspeexdsp.1.dylib $PACKED_BUILD/obs-plugins/obs-filters.so
sudo install_name_tool -change /tmp/obsdeps/lib/librnnoise.0.dylib  @executable_path/librnnoise.0.dylib $PACKED_BUILD/obs-plugins/obs-filters.so

sudo install_name_tool -change /tmp/obsdeps/lib/libavcodec.58.dylib @executable_path/libavcodec.58.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavfilter.7.dylib @executable_path/libavfilter.7.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavdevice.58.dylib @executable_path/libavdevice.58.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavutil.56.dylib @executable_path/libavutil.56.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/lib/libswscale.5.dylib @executable_path/libswscale.5.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/lib/libavformat.58.dylib @executable_path/libavformat.58.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so
sudo install_name_tool -change /tmp/obsdeps/lib/libswresample.3.dylib @executable_path/libswresample.3.dylib $PACKED_BUILD/obs-plugins/slobs-virtual-cam.so


sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.13.dylib @executable_path/libmbedtls.13.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.1.dylib @executable_path/libmbedx509.1.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.5.dylib @executable_path/libmbedcrypto.5.dylib $PACKED_BUILD/bin/libavformat.58.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.13.dylib @executable_path/libmbedtls.13.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.1.dylib @executable_path/libmbedx509.1.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.5.dylib @executable_path/libmbedcrypto.5.dylib $PACKED_BUILD/bin/libavdevice.58.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.13.dylib @executable_path/libmbedtls.13.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.1.dylib @executable_path/libmbedx509.1.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.5.dylib @executable_path/libmbedcrypto.5.dylib $PACKED_BUILD/bin/libavfilter.7.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libmbedx509.1.dylib @executable_path/libmbedx509.1.dylib $PACKED_BUILD/bin/libmbedtls.13.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.5.dylib @executable_path/libmbedcrypto.5.dylib $PACKED_BUILD/bin/libmbedtls.13.dylib
sudo install_name_tool -change /tmp/obsdeps/lib/libmbedtls.2.24.0.dylib @executable_path/libmbedtls.2.24.0.dylib $PACKED_BUILD/bin/libmbedtls.13.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libmbedcrypto.5.dylib @executable_path/libmbedcrypto.5.dylib $PACKED_BUILD/bin/libmbedx509.1.dylib

sudo install_name_tool -change /tmp/obsdeps/lib/libx264.161.dylib @executable_path/libx264.161.dylib $PACKED_BUILD/obs-plugins/obs-x264.so

sudo install_name_tool -change $DEPS_DIR/obsdeps/lib/libfreetype.6.dylib @executable_path/libfreetype.6.dylib $PACKED_BUILD/obs-plugins/text-freetype2.so

sudo install_name_tool -change /usr/local/opt/xz/lib/liblzma.5.dylib @executable_path/liblzma.5.dylib $PACKED_BUILD/bin/libavcodec.58.dylib
sudo install_name_tool -change /usr/local/opt/xz/lib/liblzma.5.dylib @executable_path/liblzma.5.dylib $PACKED_BUILD/bin/libavformat.58.dylib
sudo install_name_tool -change /usr/local/opt/xz/lib/liblzma.5.dylib @executable_path/liblzma.5.dylib $PACKED_BUILD/bin/libavfilter.7.dylib
sudo install_name_tool -change /usr/local/opt/xz/lib/liblzma.5.dylib @executable_path/liblzma.5.dylib $PACKED_BUILD/bin/libavdevice.58.dylib
