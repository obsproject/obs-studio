curl -L -O https://s3-us-west-2.amazonaws.com/obs-nightly/Packages.pkg
sudo installer -pkg ./Packages.pkg -target /

brew update

#Base OBS Deps
brew install qt5

curl -L -O https://s3-us-west-2.amazonaws.com/obs-nightly/osx-deps.tar.gz
tar -xf ./osx-deps.tar.gz -C /tmp

# CEF Stuff
cd ../
curl -kLO http://opensource.spotify.com/cefbuilds/cef_binary_3.2883.1540.gedbfb20_macosx64.tar.bz2
tar -xf ./cef_binary_3.2883.1540.gedbfb20_macosx64.tar.bz2
cd ./cef_binary_3.2883.1540.gedbfb20_macosx64
mkdir build
cd ./build
cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 ..
make -j4
mkdir libcef_dll
cd ../../
