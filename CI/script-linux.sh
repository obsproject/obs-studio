#!/bin/sh
set -ex

cd ./build
make -j$(nproc)
make DESTDIR=appdir -j$(nproc) install ; find appdir/

wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
unset QTDIR; unset QT_PLUGIN_PATH ; unset LD_LIBRARY_PATH
export VERSION=$(git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
which qmake
./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage -verbose=2 -qmake=/usr/lib/x86_64-linux-gnu/qt5/bin/qmake

find appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
curl --upload-file OBS*.AppImage https://transfer.sh/OBS-git.$(git rev-parse --short HEAD)-x86_64.AppImage
