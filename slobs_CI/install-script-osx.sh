echo "RUNNING INSTALL SCRIPT"

# Echo all commands before executing
set -v

cd ../

brew update

brew install ccache mbedtls ffmpeg x264 cmake p7zip

export PATH=/usr/local/opt/ccache/libexec:$PATH
ccache -s || echo "CCache is not available."

# Fetch and untar prebuilt OBS deps that are compatible with older versions of OSX
hr "Downloading OBS deps"
wget --quiet --retry-connrefused --waitretry=1 https://obs-nightly.s3.amazonaws.com/osx-deps-2018-08-09.tar.gz
tar -xf ./osx-deps-2018-08-09.tar.gz -C /tmp

# Fetch vlc codebase
hr "Downloading VLC repo"
wget --quiet --retry-connrefused --waitretry=1 https://downloads.videolan.org/vlc/3.0.4/vlc-3.0.4.tar.xz
tar -xf vlc-3.0.4.tar.xz