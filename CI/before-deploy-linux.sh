#!/bin/sh
set -ex

export GIT_TAG=$(git describe --abbrev=0)
export VERSION=$GIT_TAG
export QT_SELECT=qt5

echo "git tag: $GIT_TAG"

cd ./build

wget -c https://github.com/$(wget -q https://github.com/probonopd/appstream/releases -O - | grep "appstreamcli-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2) -O appstreamcli
sudo chmod +x appstreamcli
export PATH=$(readlink -f .):$PATH

# Install to AppDir
cd appdir/usr
sudo ln -s lib/obs-scripting/* .
cd ../../ # FIXME: https://github.com/obsproject/obs-studio/pull/1565#issuecomment-448754477
sudo cp appdir/usr/share/icons/hicolor/256x256/apps/com.obsproject.Studio.png appdir/

sudo cp /usr/lib/x86_64-linux-gnu/libnss3.so appdir/usr/lib/x86_64-linux-gnu
sudo mkdir appdir/usr/lib/x86_64-linux-gnu/nss
sudo cp /usr/lib/x86_64-linux-gnu/nss/* appdir/usr/lib/x86_64-linux-gnu/nss

sudo apt-get -y download libpython3.6-minimal libpython3.6-stdlib
cd ./appdir
sudo dpkg -x ../libpython3.6-minimal*.deb .
sudo dpkg -x ../libpython3.6-stdlib*.deb .
cd ..

# libobs.so.0 loads resources from a path relative to cwd that only works
# when -DUNIX_STRUCTURE=0 is used at configure time, which we are not using;
# hence patching it to load resources relative to cwd = usr/
sudo sed -i -e 's|../../obs-plugins/64bit|././././lib/obs-plugins|g' appdir/usr/lib/libobs.so.0

# Workaround for:
# com.obsproject.Studio.appdata.xml
# W: com.obsproject.Studio:21: invalid-iso8601-date 
# Validation failed: warnings: 1, pedantic: 1
# ERROR: AppStream metainfo file file contains errors. Please fix them. Please see https://www.freedesktop.org/software/appstream/docs/chap-Quickstart.html#sect-Quickstart-DesktopApps
# In case of questions regarding the validation, please refer to https://github.com/ximion/appstream
sudo rm -rf appdir/usr/share/metainfo

wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
sudo chmod +x appimagetool-*.AppImage
sudo ./appimagetool-*.AppImage deploy appdir/usr/share/applications/*.desktop
sudo ./appimagetool-*.AppImage appdir/

sudo mv OBS*.AppImage ../nightly

cd ..
