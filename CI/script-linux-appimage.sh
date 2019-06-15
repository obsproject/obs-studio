#!/bin/sh
set -ex

export QT_SELECT=qt5

# Compile and install to an AppDir
cd ./build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
make DESTDIR=appdir -j$(nproc) install ; find appdir/
( cd appdir/usr ; ln -s lib/obs-scripting/* . ) # FIXME: https://github.com/obsproject/obs-studio/pull/1565#issuecomment-448754477
( cd appdir/usr ; ln -s plugins/platforms . ) # Why? linuxdeployqt bug?
find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

# Finalize the AppDir and convert it to AppImage
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
# The libraries passed in with -executable are loaded by obs with dlopen(), so linuxdeployqt can't know about them
./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -bundle-non-qt-libs \
-qmake=/usr/lib/x86_64-linux-gnu/qt5/bin/qmake \
-executable=appdir/usr/lib/obs-plugins/frontend-tools.so \
-executable=appdir/usr/lib/obs-plugins/image-source.so \
-executable=appdir/usr/lib/obs-plugins/linux-alsa.so \
-executable=appdir/usr/lib/obs-plugins/linux-capture.so \
-executable=appdir/usr/lib/obs-plugins/linux-decklink.so \
-executable=appdir/usr/lib/obs-plugins/linux-jack.so \
-executable=appdir/usr/lib/obs-plugins/linux-pulseaudio.so \
-executable=appdir/usr/lib/obs-plugins/linux-v4l2.so \
-executable=appdir/usr/lib/obs-plugins/obs-ffmpeg.so \
-executable=appdir/usr/lib/obs-plugins/obs-filters.so \
-executable=appdir/usr/lib/obs-plugins/obs-libfdk.so \
-executable=appdir/usr/lib/obs-plugins/obs-outputs.so \
-executable=appdir/usr/lib/obs-plugins/obs-transitions.so \
-executable=appdir/usr/lib/obs-plugins/obs-x264.so \
-executable=appdir/usr/lib/obs-plugins/rtmp-services.so \
-executable=appdir/usr/lib/obs-plugins/text-freetype2.so \
-executable=appdir/usr/lib/obs-plugins/vlc-video.so \
-executable=appdir/usr/lib/libobs-opengl.so.0 \
-executable=appdir/usr/bin/obs-ffmpeg-mux

# Also deploy the Python standard library
apt-get -y download libpython3.5-minimal libpython3.5-stdlib
( cd appdir ; dpkg -x ../libpython3.5-minimal*.deb . )
( cd appdir ; dpkg -x ../libpython3.5-stdlib*.deb . )

# libobs.so.0 loads resources from a path relative to cwd that only works
# when -DUNIX_STRUCTURE=0 is used at configure time, which we are not using;
# hence patching it to load resources relative to cwd = usr/
sed -i -e 's|../../obs-plugins/64bit|././././lib/obs-plugins|g' appdir/usr/lib/libobs.so.0

rm appdir/AppRun ; cp ../CI/install/AppDir/AppRun appdir/AppRun ; chmod +x appdir/AppRun # custom AppRun
./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage -verbose=2

# TODO: The next 2 lines could be replaced by a native upload mechanism defined in .travis.yml
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
bash upload.sh OBS*.AppImage*
