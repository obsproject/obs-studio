hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

# Echo all commands before executing
set -v

if [[ $TRAVIS ]]; then
  git fetch --unshallow
fi

git fetch origin --tags

# Leave obs-studio folder
cd ../

# Install Packages app so we can build a package later
# http://s.sudre.free.fr/Software/Packages/about.html
hr "Downloading Packages app"
wget --quiet --retry-connrefused --waitretry=1 https://s3-us-west-2.amazonaws.com/obs-nightly/Packages.pkg
sudo installer -pkg ./Packages.pkg -target /

brew update

#Base OBS Deps and ccache
brew install jack speexdsp ccache mbedtls freetype fdk-aac
brew install https://gist.githubusercontent.com/DDRBoxman/9c7a2b08933166f4b61ed9a44b242609/raw/ef4de6c587c6bd7f50210eccd5bd51ff08e6de13/qt.rb
if [ -d "$(brew --cellar)/swig" ]; then
    brew unlink swig
fi
brew install https://gist.githubusercontent.com/DDRBoxman/4cada55c51803a2f963fa40ce55c9d3e/raw/572c67e908bfbc1bcb8c476ea77ea3935133f5b5/swig.rb

pip install dmgbuild

export PATH=/usr/local/opt/ccache/libexec:$PATH
ccache -s || echo "CCache is not available."

# Fetch and untar prebuilt OBS deps that are compatible with older versions of OSX
hr "Downloading OBS deps"
wget --quiet --retry-connrefused --waitretry=1 https://github.com/obsproject/obs-deps/releases/download/2020-04-24/osx-deps-2020-04-24.tar.gz
tar -xf ./osx-deps-2020-04-24.tar.gz -C /tmp

# Fetch vlc codebase
hr "Downloading VLC repo"
wget --quiet --retry-connrefused --waitretry=1 https://downloads.videolan.org/vlc/3.0.8/vlc-3.0.8.tar.xz
tar -xf vlc-3.0.8.tar.xz

# Get sparkle
hr "Downloading Sparkle framework"
wget --quiet --retry-connrefused --waitretry=1 -O sparkle.tar.bz2 https://github.com/sparkle-project/Sparkle/releases/download/1.23.0/Sparkle-1.23.0.tar.bz2
mkdir ./sparkle
tar -xf ./sparkle.tar.bz2 -C ./sparkle
sudo cp -R ./sparkle/Sparkle.framework /Library/Frameworks/Sparkle.framework

# CEF Stuff
hr "Downloading CEF"
wget --quiet --retry-connrefused --waitretry=1 https://obs-nightly.s3-us-west-2.amazonaws.com/cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2
tar -xf ./cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2
cd ./cef_binary_${CEF_BUILD_VERSION}_macosx64
# remove a broken test
sed -i '.orig' '/add_subdirectory(tests\/ceftests)/d' ./CMakeLists.txt
# target 10.11
sed -i '.orig' s/\"10.9\"/\"10.11\"/ ./cmake/cef_variables.cmake
mkdir build
cd ./build
cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 ..
make -j4
mkdir libcef_dll
cd ../../
