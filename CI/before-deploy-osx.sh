hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

# Generate file name variables
export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-$TRAVIS_BRANCH-osx.pkg

cd ./build

# Move the CEF plugin out before running build_app so that it doesn't get packaged twice
hr "Moving CEF out to preserve linking"
mv ./rundir/RelWithDebInfo/obs-plugins/CEF.app ./
mv ./rundir/RelWithDebInfo/obs-plugins/obs-browser.so ./

# Package everything into a nice .app
hr "Packaging .app"
STABLE=false
if [ -n "${TRAVIS_TAG}" ]; then
  STABLE=true
fi

sudo python ../CI/install/osx/build_app.py --public-key ../CI/install/osx/OBSPublicDSAKey.pem --sparkle-framework ../../sparkle/Sparkle.framework --base-url "https://obsproject.com/osx_update" --stable=$STABLE

# Move the CEF plugin back to where it belongs
hr "Moving CEF back"
mv ./CEF.app ./rundir/RelWithDebInfo/obs-plugins/
mv ./obs-browser.so ./rundir/RelWithDebInfo/obs-plugins/

# Package app
hr "Generating .pkg"
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

# Signing stuff
hr "Decrypting Cert"
openssl aes-256-cbc -K $encrypted_dd3c7f5e9db9_key -iv $encrypted_dd3c7f5e9db9_iv -in ../CI/osxcert/Certificates.p12.enc -out Certificates.p12 -d
hr "Creating Keychain"
security create-keychain -p mysecretpassword build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p mysecretpassword build.keychain
security set-keychain-settings -t 3600 -u build.keychain
hr "Importing certs into keychain"
security import ./Certificates.p12 -k build.keychain -T /usr/bin/productsign -P ""
# macOS 10.12+
security set-key-partition-list -S apple-tool:,apple: -s -k mysecretpassword build.keychain
hr "Signing Package"
productsign --sign 2MMRE5MTB8 ./OBS.pkg ./$FILENAME

# Move to the folder that travis uses to upload artifacts from
hr "Moving package to nightly folder for distribution"
mkdir ../nightly
sudo mv ./$FILENAME ../nightly
