brew update

#Base OBS Deps
brew install ffmpeg x264 qt5

# CEF Stuff
cd ../
curl -kLO http://opensource.spotify.com/cefbuilds/cef_binary_3.2704.1434.gec3e9ed_macosx64.tar.bz2
tar -xf ./cef_binary_3.2704.1434.gec3e9ed_macosx64.tar.bz2
cd ./cef_binary_3.2704.1434.gec3e9ed_macosx64
mkdir build
cd ./build
cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" ..
make -j4
mkdir libcef_dll
mv ./libcef_dll_wrapper/libcef_dll_wrapper.a ./libcef_dll/libcef_dll_wrapper.a
cd ../../
