export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-osx.pkg
mkdir nightly
cd ./build
mv ./rundir/RelWithDebInfo/obs-plugins/CEF.app ./
mv ./rundir/RelWithDebInfo/obs-plugins/obs-browser.so ./
sudo python ../CI/install/osx/build_app.py
mv ./CEF.app ./rundir/RelWithDebInfo/obs-plugins/
mv ./obs-browser.so ./rundir/RelWithDebInfo/obs-plugins/
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

sudo cp OBS.pkg ./$FILENAME
sudo mv ./$FILENAME ../nightly
