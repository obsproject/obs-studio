# Exit if something fails
set -e

# Generate file name variables
export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-osx.pkg

cd ./build

# Move the CEF plugin out before running build_app so that it doesn't get packaged twice
mv ./rundir/RelWithDebInfo/obs-plugins/CEF.app ./
mv ./rundir/RelWithDebInfo/obs-plugins/obs-browser.so ./

# Package everything into a nice .app
sudo python ../CI/install/osx/build_app.py --public-key ../CI/install/osx/OBSPublicDSAKey.pem --sparkle-framework ../../sparkle/Sparkle.framework #--base-url=https://obsappcasturlhere

# Move the CEF plugin back to where it belongs
mv ./CEF.app ./rundir/RelWithDebInfo/obs-plugins/
mv ./obs-browser.so ./rundir/RelWithDebInfo/obs-plugins/

# Package app
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

# Signing stuff
openssl aes-256-cbc -K $encrypted_dd3c7f5e9db9_key -iv $encrypted_dd3c7f5e9db9_iv -in ../CI/osxcert/cert.p12.enc -out cert.p12 -d
openssl aes-256-cbc -K $encrypted_dd3c7f5e9db9_key -iv $encrypted_dd3c7f5e9db9_iv -in ../CI/osxcert/pk.p12.enc -out pk.p12 -d
security create-keychain -p mysecretpassword build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p mysecretpassword build.keychain
security import cert.p12 -k build.keychain -T /usr/bin/codesign
security import pk.p12 -k build.keychain -T /usr/bin/codesign
productsign --sign 'Developer ID Installer: Hugh Bailey (2MMRE5MTB8)' ./OBS.pkg ./$FILENAME

# Move to the folder that travis uses to upload artifacts from
mkdir ../nightly
sudo mv ./$FILENAME ../nightly
