curl -L -O https://s3-us-west-2.amazonaws.com/obs-nightly/packagesbuild
chmod +x ./packagesbuild
curl -L -O https://s3-us-west-2.amazonaws.com/obs-nightly/packagesutil
chmod +x ./packagesutil

export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-osx.pkg
mkdir nightly
cd ./build
sudo python ../CI/install/osx/build_app.py
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

sudo cp OBS.pkg ./$FILENAME
sudo mv ./$FILENAME ../nightly
