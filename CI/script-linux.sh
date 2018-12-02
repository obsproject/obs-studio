#!/bin/sh
set -ex

# Compile and install to an AppDir
export LD_LIBRARY_PATH=/opt/qt59/lib/x86_64-linux-gnu:/opt/qt59/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/opt/qt59/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=/opt/qt59/bin:$PATH
cd ./build
make -j$(nproc)
make DESTDIR=appdir -j$(nproc) install ; find appdir/
( cd appdir/usr ; ln -s lib/obs-scripting/* . ) # FIXME: https://github.com/obsproject/obs-studio/pull/1565#issuecomment-448754477
find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

# Finalize the AppDir and convert it to AppImage
wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
export VERSION=$(git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
# The libraries passed in with -executable are loaded by obs with dlopen(), so linuxdeployqt can't know about them
./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -bundle-non-qt-libs \
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
-executable=appdir/usr/share/obs/obs-plugins/obs-ffmpeg/ffmpeg-mux

# Also deploy the Python standard library
( cd appdir ; dpkg -x /var/cache/apt/archives/libpython3.4-minimal*.deb . )
( cd appdir ; dpkg -x /var/cache/apt/archives/libpython3.4-stdlib*.deb . )

# libobs.so.0 loads resources from a path relative to cwd that only works
# when -DUNIX_STRUCTURE=0 is used at configure time, which we are not using;
# hence patching it to load resources relative to cwd = usr/
sed -i -e 's|../../obs-plugins/64bit|././././lib/obs-plugins|g' appdir/usr/lib/libobs.so.0

rm appdir/AppRun ; cp ../CI/install/AppDir/AppRun appdir/AppRun ; chmod +x appdir/AppRun # custom AppRun
./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage

# TODO: The next line should be replaced by a native upload mechanism defined in .travis.yml,
# e.g., using https://github.com/probonopd/uploadtool
curl --upload-file OBS*.AppImage https://transfer.sh/OBS-git.$(git rev-parse --short HEAD)-x86_64.AppImage
