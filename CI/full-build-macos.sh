#!/usr/bin/env bash

##############################################################################
# macOS full build script
##############################################################################
#
# This script contains all steps necessary to:
#
#   * Build OBS with all default plugins and dependencies
#   * Create a macOS application bundle
#   * Code-sign the macOS application-bundle
#   * Package a macOS installation image
#   * Notarize macOS application-bundle and/or installation image
#
# Parameters:
#   -b: Create macOS bundle
#   -p: Create macOS distribution image
#   -n: Notarize macOS app and disk image (implies bundling)
#   -s: Skip the build process (useful for bundling/packaging only)
#   -h: Print usage help
#
# Environment Variables (optional):
#   MACOS_DEPS_VERSION  : Pre-compiled macOS dependencies version
#   CEF_BUILD_VERSION   : Chromium Embedded Framework version
#   VLC_VERISON         : VLC version
#   SPARKLE_VERSION     : Sparke Framework version
#
##############################################################################

# Halt on errors
set -eE

## SET UP ENVIRONMENT ##
PRODUCT_NAME="OBS-Studio"

CHECKOUT_DIR="$(git rev-parse --show-toplevel)"
DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
CI_SCRIPTS="${CHECKOUT_DIR}/CI/scripts/macos"
CI_WORKFLOW="${CHECKOUT_DIR}/.github/workflows/main.yml"
CI_CEF_VERSION=$(cat ${CI_WORKFLOW} | sed -En "s/[ ]+CEF_BUILD_VERSION: '([0-9]+)'/\1/p")
CI_DEPS_VERSION=$(cat ${CI_WORKFLOW} | sed -En "s/[ ]+MACOS_DEPS_VERSION: '([0-9\-]+)'/\1/p")
CI_VLC_VERSION=$(cat ${CI_WORKFLOW} | sed -En "s/[ ]+VLC_VERSION: '([0-9\.]+)'/\1/p")
CI_SPARKLE_VERSION=$(cat ${CI_WORKFLOW} | sed -En "s/[ ]+SPARKLE_VERSION: '([0-9\.]+)'/\1/p")
CI_QT_VERSION=$(cat ${CI_WORKFLOW} | sed -En "s/[ ]+QT_VERSION: '([0-9\.]+)'/\1/p" | head -1)

BUILD_DEPS=(
    "obs-deps ${MACOS_DEPS_VERSION:-${CI_DEPS_VERSION}}"
    "cef ${CEF_BUILD_VERSION:-${CI_CEF_VERSION}}"
    "vlc ${VLC_VERSION:-${CI_VLC_VERSION}}"
    "sparkle ${SPARKLE_VERSION:-${CI_SPARKLE_VERSION}}"
)

COLOR_RED=$(tput setaf 1)
COLOR_GREEN=$(tput setaf 2)
COLOR_BLUE=$(tput setaf 4)
COLOR_ORANGE=$(tput setaf 3)
COLOR_RESET=$(tput sgr0)

## DEFINE UTILITIES ##

hr() {
    echo -e "${COLOR_BLUE}[${PRODUCT_NAME}] ${1}${COLOR_RESET}"
}

step() {
    echo -e "${COLOR_GREEN}  + ${1}${COLOR_RESET}"
}

info() {
    echo -e "${COLOR_ORANGE} + ${1}${COLOR_RESET}"
}

error() {
    echo -e "${COLOR_RED}  + ${1}${COLOR_RESET}"
}

exists() {
  command -v "$1" >/dev/null 2>&1
}

ensure_dir() {
    [[ -n ${1} ]] && mkdir -p ${1} && builtin cd ${1}
}

cleanup() {
    rm -rf "${CHECKOUT_DIR}/build/settings.json"
    unset CODESIGN_IDENT
    unset CODESIGN_IDENT_USER
    unset CODESIGN_IDENT_PASS
}

caught_error() {
    error "ERROR during build step: ${1}"
    cleanup
    exit 1
}

## CHECK AND INSTALL DEPENDENCIES ##
install_homebrew_deps() {
    if ! exists brew; then
        error "Homebrew not found - please install homebrew (https://brew.sh)"
        exit 1
    fi

    info "Specific Swig and Qt versions will be installed and *pinned* on your system."

    if [ -d "$(brew --cellar)/swig" ]; then
         brew unlink swig
    fi

    if [ -d "$(brew --cellar)/qt" ]; then
         brew unlink qt
    fi

    brew bundle --file ${CI_SCRIPTS}/Brewfile
    brew pin qt
    brew pin swig
}

check_ccache() {
    export PATH=/usr/local/opt/ccache/libexec:${PATH}
    CCACHE_STATUS=$(ccache -s >/dev/null 2>&1 && echo "CCache available." || echo "CCache is not available.")
    info "${CCACHE_STATUS}"
}

install_obs-deps() {
    hr "Setting up pre-built macOS OBS dependencies v${1}"
    ensure_dir ${DEPS_BUILD_DIR}
    step "Download..."
    curl -s -L -C - -O https://github.com/obsproject/obs-deps/releases/download/${1}/osx-deps-${1}.tar.gz
    step "Unpack..."
    tar -xf ./osx-deps-${1}.tar.gz -C /tmp
}

install_vlc() {
    hr "Setting up dependency VLC v${1}"
    ensure_dir ${DEPS_BUILD_DIR}
    step "Download..."
    curl -s -L -C - -O https://downloads.videolan.org/vlc/${1}/vlc-${1}.tar.xz
    step "Unpack ..."
    tar -xf vlc-${1}.tar.xz
}

install_sparkle() {
    hr "Setting up dependency Sparkle v${1} (might prompt for password)"
    ensure_dir ${DEPS_BUILD_DIR}/sparkle
    step "Download..."
    curl -s -L -C - -o sparkle.tar.bz2 https://github.com/sparkle-project/Sparkle/releases/download/${1}/Sparkle-${1}.tar.bz2
    step "Unpack..."
    tar -xf ./sparkle.tar.bz2
    step "Copy to destination..."
    if [ -d /Library/Frameworks/Sparkle.framework/ ]; then
        info "Warning - Sparkle framework already found in /Library/Frameworks"
    else
        sudo cp -R ./Sparkle.framework/ /Library/Frameworks/Sparkle.framework/
    fi
}

install_cef() {
    hr "Building dependency CEF v${1}"
    ensure_dir ${DEPS_BUILD_DIR}
    step "Download..."
    curl -s -L -C - -O https://obs-nightly.s3-us-west-2.amazonaws.com/cef_binary_${1}_macosx64.tar.bz2
    step "Unpack..."
    tar -xf ./cef_binary_${1}_macosx64.tar.bz2
    cd ./cef_binary_${1}_macosx64
    step "Fix tests..."
    # remove a broken test
    sed -i '.orig' '/add_subdirectory(tests\/ceftests)/d' ./CMakeLists.txt
    # target 10.11
    sed -i '.orig' s/\"10.9\"/\"10.11\"/ ./cmake/cef_variables.cmake
    ensure_dir ./build
    step "Run CMAKE..."
    cmake \
        -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++"\
        -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++"\
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 \
        ..
    step "Build..."
    make -j4
    if [ ! -d libcef_dll ]; then mkdir libcef_dll; fi
}

## CHECK AND INSTALL PACKAGING DEPENDENCIES ##
install_packages_app() {
    if [ ! -d /Applications/Packages.app ]; then
        hr "Installing Packages app"
        ensure_dir ${DEPS_BUILD_DIR}
        step "Download..."
        curl -s -L -C - -O https://s3-us-west-2.amazonaws.com/obs-nightly/Packages.pkg
        step "Install..."
        sudo installer -pkg ./Packages.pkg -target /
    fi
}

install_dmgbuild() {
    if ! exists dmgbuild; then
        if exists "pip3"; then
            PIPCMD="pip3"
        elif exists "pip"; then
            PIPCMD="pip"
        else
            error "Pip not found - please install pip via 'python -m ensurepip'"
            exit 1
        fi

        ${PIPCMD} install dmgbuild
    fi
}

## OBS BUILD FROM SOURCE ##
configure_obs_build() {
    ensure_dir "${CHECKOUT_DIR}/build"

    CUR_DATE=$(date +"%Y-%m-%d@%H%M%S")
    NIGHTLY_DIR="${CHECKOUT_DIR}/nightly-${CUR_DATE}"
    PACKAGE_NAME=$(find . -name "*.dmg")

    if [ -d ./OBS.app ]; then
        ensure_dir "${NIGHTLY_DIR}"
        mv ../build/OBS.app .
        info "You can find OBS.app in ${NIGHTLY_DIR}"
    fi
    ensure_dir "${CHECKOUT_DIR}/build"
    if ([ -n "${PACKAGE_NAME}" ] && [ -f ${PACKAGE_NAME} ]); then
        ensure_dir "${NIGHTLY_DIR}"
        mv ../build/$(basename "${PACKAGE_NAME}") .
        info "You can find ${PACKAGE_NAME} in ${NIGHTLY_DIR}"
    fi

    ensure_dir "${CHECKOUT_DIR}/build"

    hr "Run CMAKE for OBS..."
    cmake -DENABLE_SPARKLE_UPDATER=ON \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 \
        -DQTDIR="/usr/local/Cellar/qt/${QT_VERSION:-${CI_QT_VERSION}}" \
        -DDepsPath=/tmp/obsdeps \
        -DVLCPath="${DEPS_BUILD_DIR}/vlc-${VLC_VERSION:-${CI_VLC_VERSION}}" \
        -DBUILD_BROWSER=ON \
        -DBROWSER_DEPLOY=ON \
        -DBUILD_CAPTIONS=ON \
        -DWITH_RTMPS=ON \
        -DCEF_ROOT_DIR="${DEPS_BUILD_DIR}/cef_binary_${CEF_BUILD_VERSION:-${CI_CEF_VERSION}}_macosx64" \
        ..
}

run_obs_build() {
    ensure_dir "${CHECKOUT_DIR}/build"
    hr "Build OBS..."
    make -j4
}

## OBS BUNDLE AS MACOS APPLICATION ##
bundle_dylibs() {
    ensure_dir "${CHECKOUT_DIR}/build"

    if [ ! -d ./OBS.app ]; then
        error "No OBS.app bundle found"
        exit 1
    fi

    hr "Bundle dylibs for macOS application"

    step "Fix mbedtls for obs-outputs..."
    install_name_tool -change libmbedtls.12.dylib @executable_path/../Frameworks/libmbedtls.12.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
    install_name_tool -change libmbedcrypto.3.dylib @executable_path/../Frameworks/libmbedcrypto.3.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
    install_name_tool -change libmbedx509.0.dylib @executable_path/../Frameworks/libmbedx509.0.dylib ./OBS.app/Contents/Plugins/obs-outputs.so

    step "Run dylibBundler.."
    ${CI_SCRIPTS}/app/dylibBundler -cd -of -a ./OBS.app -q -f \
        -s ./OBS.app/Contents/MacOS \
        -s "${DEPS_BUILD_DIR}/sparkle/Sparkle.framework" \
        -x ./OBS.app/Contents/PlugIns/coreaudio-encoder.so \
        -x ./OBS.app/Contents/PlugIns/decklink-ouput-ui.so \
        -x ./OBS.app/Contents/PlugIns/frontend-tools.so \
        -x ./OBS.app/Contents/PlugIns/image-source.so \
        -x ./OBS.app/Contents/PlugIns/linux-jack.so \
        -x ./OBS.app/Contents/PlugIns/mac-avcapture.so \
        -x ./OBS.app/Contents/PlugIns/mac-capture.so \
        -x ./OBS.app/Contents/PlugIns/mac-decklink.so \
        -x ./OBS.app/Contents/PlugIns/mac-syphon.so \
        -x ./OBS.app/Contents/PlugIns/mac-vth264.so \
        -x ./OBS.app/Contents/PlugIns/obs-browser.so \
        -x ./OBS.app/Contents/PlugIns/obs-browser-page \
        -x ./OBS.app/Contents/PlugIns/obs-ffmpeg.so \
        -x ./OBS.app/Contents/PlugIns/obs-filters.so \
        -x ./OBS.app/Contents/PlugIns/obs-transitions.so \
        -x ./OBS.app/Contents/PlugIns/obs-vst.so \
        -x ./OBS.app/Contents/PlugIns/rtmp-services.so \
        -x ./OBS.app/Contents/MacOS/obs-ffmpeg-mux \
        -x ./OBS.app/Contents/MacOS/obslua.so \
        -x ./OBS.app/Contents/MacOS/_obspython.so \
        -x ./OBS.app/Contents/PlugIns/obs-x264.so \
        -x ./OBS.app/Contents/PlugIns/text-freetype2.so \
        -x ./OBS.app/Contents/PlugIns/obs-libfdk.so \
        -x ./OBS.app/Contents/PlugIns/obs-outputs.so
    step "Move libobs-opengl to final destination"
    mv ./OBS.app/Contents/MacOS/libobs-opengl.so ./OBS.app/Contents/Frameworks
}

install_frameworks() {
    ensure_dir "${CHECKOUT_DIR}/build"

    if [ ! -d ./OBS.app ]; then
        error "No OBS.app bundle found"
        exit 1
    fi

    hr "Adding Chromium Embedded Framework"
    step "Copy Framework..."
    sudo cp -R "${DEPS_BUILD_DIR}/cef_binary_${CEF_BUILD_VERSION:-${CI_CEF_VERSION}}_macosx64/Release/Chromium Embedded Framework.framework" ./OBS.app/Contents/Frameworks/
    sudo chown -R $(whoami) ./OBS.app/Contents/Frameworks/
    step "Fix dylib references in obs-browser.so"
    install_name_tool -change /usr/local/Cellar/qt/${QT_VERSION:-${CI_QT_VERSION}}/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./OBS.app/Contents/Plugins/obs-browser.so
    install_name_tool -change /usr/local/Cellar/qt/${QT_VERSION:-${CI_QT_VERSION}}/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./OBS.app/Contents/Plugins/obs-browser.so
    install_name_tool -change /usr/local/Cellar/qt/${QT_VERSION:-${CI_QT_VERSION}}/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./OBS.app/Contents/Plugins/obs-browser.so
}

prepare_macos_bundle() {
    ensure_dir "${CHECKOUT_DIR}/build"

    if [ ! -d ./rundir/RelWithDebInfo/bin ]; then
        error "No OBS build found"
        return
    fi

    if [ -d ./OBS.app ]; then rm -rf ./OBS.app; fi

    hr "Preparing OBS.app bundle"
    step "Copy binary and plugins..."
    mkdir -p OBS.app/Contents/MacOS
    mkdir OBS.app/Contents/PlugIns
    mkdir OBS.app/Contents/Resources

    cp -R rundir/RelWithDebInfo/bin/ ./OBS.app/Contents/MacOS
    cp -R rundir/RelWithDebInfo/data ./OBS.app/Contents/Resources
    cp ${CI_SCRIPTS}/app/obs.icns ./OBS.app/Contents/Resources
    cp -R rundir/RelWithDebInfo/obs-plugins/ ./OBS.app/Contents/PlugIns
    cp ${CI_SCRIPTS}/app/Info.plist ./OBS.app/Contents
    # Scripting plugins are required to be placed in same directory as binary
    if [ -d ./OBS.app/Contents/Resources/data/obs-scripting ]; then
        mv ./OBS.app/Contents/Resources/data/obs-scripting/obslua.so ./OBS.app/Contents/MacOS/
        mv ./OBS.app/Contents/Resources/data/obs-scripting/_obspython.so ./OBS.app/Contents/MacOS/
        mv ./OBS.app/Contents/Resources/data/obs-scripting/obspython.py ./OBS.app/Contents/MacOS/
        rm -rf ./OBS.app/Contents/Resources/data/obs-scripting/
    fi

    bundle_dylibs
    install_frameworks

    cp ${CI_SCRIPTS}/app/OBSPublicDSAKey.pem ./OBS.app/Contents/Resources

    step "Set bundle meta information..."
    plutil -insert CFBundleVersion -string ${GIT_TAG}-${GIT_HASH} ./OBS.app/Contents/Info.plist
    plutil -insert CFBundleShortVersionString -string ${GIT_TAG}-${GIT_HASH} ./OBS.app/Contents/Info.plist
    plutil -insert OBSFeedsURL -string https://obsproject.com/osx_update/feeds.xml ./OBS.app/Contents/Info.plist
    plutil -insert SUFeedURL -string https://obsproject.com/osx_update/stable/updates.xml ./OBS.app/Contents/Info.plist
    plutil -insert SUPublicDSAKeyFile -string OBSPublicDSAKey.pem ./OBS.app/Contents/Info.plist
}

## CREATE MACOS DISTRIBUTION AND INSTALLER IMAGE ##
prepare_macos_image() {
    ensure_dir "${CHECKOUT_DIR}/build"

    if [ ! -d ./OBS.app ]; then
        error "No OBS.app bundle found"
        return
    fi

    hr "Preparing macOS installation image"

    if [ -f "${FILE_NAME}" ]; then
        rm "${FILE_NAME}"
    fi

    step "Run dmgbuild..."
    cp "${CI_SCRIPTS}/package/settings.json.template" ./settings.json
    sed -i '' 's#\$\$VERSION\$\$#'"${GIT_TAG}"'#g' ./settings.json
    sed -i '' 's#\$\$CI_PATH\$\$#'"${CI_SCRIPTS}"'#g' ./settings.json
    sed -i '' 's#\$\$BUNDLE_PATH\$\$#'"${CHECKOUT_DIR}"'/build#g' ./settings.json
    echo -n "${COLOR_ORANGE}"
    dmgbuild "OBS-Studio ${GIT_TAG}" "${FILE_NAME}" -s ./settings.json
    echo -n "${COLOR_RESET}"

    if [ -n "${CODESIGN_OBS}" ]; then
        codesign_image
    fi
}

## SET UP CODE SIGNING AND NOTARIZATION CREDENTIALS ##
##############################################################################
# Apple Developer Identity needed:
#
#    + Signing the code requires a developer identity in the system's keychain
#    + codesign will look up and find the identity automatically
#
##############################################################################
read_codesign_ident() {
    if [ ! -n "${CODESIGN_IDENT}" ]; then
        step "Code-signing Setup"
        read -p "${COLOR_ORANGE}  + Apple developer identity: ${COLOR_RESET}" CODESIGN_IDENT
    fi
}

##############################################################################
# Apple Developer credentials necessary:
#
#   + Signing for distribution and notarization require an active Apple
#     Developer membership
#   + An Apple Development identity is needed for code signing
#     (i.e. 'Apple Development: YOUR APPLE ID (PROVIDER)')
#   + Your Apple developer ID is needed for notarization
#   + An app-specific password is necessary for notarization from CLI
#   + This password will be stored in your macOS keychain under the identifier
#     'OBS-Codesign-Password'with access Apple's 'altool' only.
##############################################################################

read_codesign_pass() {
    if [ ! -n "${CODESIGN_IDENT_PASS}" ]; then
        step "Notarization Setup"
        read -p "${COLOR_ORANGE}  + Apple account id: ${COLOR_RESET}" CODESIGN_IDENT_USER
        CODESIGN_IDENT_PASS=$(stty -echo; read -p "${COLOR_ORANGE}  + Apple developer password: ${COLOR_RESET}" pwd; stty echo; echo $pwd)
        echo -n "${COLOR_ORANGE}"
        xcrun altool --store-password-in-keychain-item "OBS-Codesign-Password" -u "${CODESIGN_IDENT_USER}" -p "${CODESIGN_IDENT_PASS}"
        echo -n "${COLOR_RESET}"
        CODESIGN_IDENT_SHORT=$(echo "${CODESIGN_IDENT}" | sed -En "s/.+\((.+)\)/\1/p")
    fi
}

codesign_bundle() {
    if [ ! -n "${CODESIGN_OBS}" ]; then step "Skipping application bundle code signing"; return; fi

    ensure_dir "${CHECKOUT_DIR}/build"
    trap "caught_error 'code-signing app'" ERR

    if [ ! -d ./OBS.app ]; then
        error "No OBS.app bundle found"
        return
    fi

    hr "Code-signing application bundle"

    xattr -crs ./OBS.app

    read_codesign_ident
    step "Code-sign Sparkle framework..."
    echo -n "${COLOR_ORANGE}"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" "./OBS.app/Contents/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/MacOS/fileop"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" "./OBS.app/Contents/Frameworks/Sparkle.framework/Versions/A/Resources/Autoupdate.app/Contents/MacOS/Autoupdate"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" --deep ./OBS.app/Contents/Frameworks/Sparkle.framework
    echo -n "${COLOR_RESET}"

    step "Code-sign CEF framework..."
    echo -n "${COLOR_ORANGE}"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" "./OBS.app/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/libEGL.dylib"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" "./OBS.app/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/libswiftshader_libEGL.dylib"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" "./OBS.app/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/libGLESv2.dylib"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" "./OBS.app/Contents/Frameworks/Chromium Embedded Framework.framework/Libraries/libswiftshader_libGLESv2.dylib"
    codesign --force --options runtime --sign "${CODESIGN_IDENT}" --deep "./OBS.app/Contents/Frameworks/Chromium Embedded Framework.framework"
    echo -n "${COLOR_RESET}"

    step "Code-sign OBS code..."
    echo -n "${COLOR_ORANGE}"
    codesign --force --options runtime --entitlements "${CI_SCRIPTS}/app/entitlements.plist" --sign "${CODESIGN_IDENT}" --deep ./OBS.app
    echo -n "${COLOR_RESET}"
    step "Check code-sign result..."
    codesign -dvv ./OBS.app
}

codesign_image() {
    if [ ! -n "${CODESIGN_OBS}" ]; then step "Skipping installer image code signing"; return; fi

    ensure_dir "${CHECKOUT_DIR}/build"
    trap "caught_error 'code-signing image'" ERR

    if [ ! -f "${FILE_NAME}" ]; then
        error "No OBS disk image found"
        return
    fi

    hr "Code-signing installation image"

    read_codesign_ident

    step "Code-sign OBS installer image..."
    echo -n "${COLOR_ORANGE}";
    codesign --force --sign "${CODESIGN_IDENT}" "${FILE_NAME}"
    echo -n "${COLOR_RESET}"
    step "Check code-sign result..."
    codesign -dvv "${FILE_NAME}"
}

## BUILD FROM SOURCE META FUNCTION ##
full-build-macos() {
    if [ -n "${SKIP_BUILD}" ]; then step "Skipping full build"; return; fi

    hr "Installing Homebrew dependencies"
    install_homebrew_deps

    for DEPENDENCY in "${BUILD_DEPS[@]}"; do
        set -- ${DEPENDENCY}
        trap "caught_error ${DEPENDENCY}" ERR
        FUNC_NAME="install_${1}"
        ${FUNC_NAME} ${2}
    done

    check_ccache
    trap "caught_error 'cmake'" ERR
    configure_obs_build
    run_obs_build
}

## BUNDLE MACOS APPLICATION META FUNCTION ##
bundle_macos() {
    if [ ! -n "${BUNDLE_OBS}" ]; then step "Skipping application bundle creation"; return; fi

    hr "Creating macOS app bundle"
    trap "caught_error 'bundle app'" ERR
    install_packages_app
    ensure_dir ${CHECKOUT_DIR}
    prepare_macos_bundle
}

## PACKAGE MACOS DISTRIBUTION IMAGE META FUNCTION ##
package_macos() {
    if [ ! -n "${PACKAGE_OBS}" ]; then step "Skipping installer image creation"; return; fi

    hr "Creating macOS .dmg image"
    trap "caught_error 'package app'" ERR

    install_dmgbuild
    prepare_macos_image
}

## NOTARIZATION META FUNCTION ##
notarize_macos() {
    if [ ! -n "${NOTARIZE_OBS}" ]; then step "Skipping macOS notarization"; return; fi;

    hr "Notarizing OBS for macOS"
    trap "caught_error 'notarizing app'" ERR

    ensure_dir "${CHECKOUT_DIR}/build"

    if [ -f "${FILE_NAME}" ]; then
        NOTARIZE_TARGET="${FILE_NAME}"
        xcnotary precheck "./OBS.app"
    elif [ -d "OBS.app" ]; then
        NOTARIZE_TARGET="./OBS.app"
    else
        error "No notarization app bundle ('OBS.app') or disk image ('${FILE_NAME}') found"
        return
    fi

    if [ "$?" -eq 0 ]; then
        read_codesign_ident
        read_codesign_pass

        step "Run xcnotary with ${NOTARIZE_TARGET}..."
        xcnotary notarize "${NOTARIZE_TARGET}" --developer-account "${CODESIGN_IDENT_USER}" --developer-password-keychain-item "OBS-Codesign-Password" --provider "${CODESIGN_IDENT_SHORT}"
    fi
}

## MAIN SCRIPT FUNCTIONS ##
print_usage() {
    echo -e "full-build-macos.sh - Build helper script for OBS-Studio\n"
    echo -e "Usage: ${0}\n" \
        "-b: Create macOS app bundle\n" \
        "-c: Codesign macOS app bundle\n" \
        "-p: Package macOS app into disk image\n" \
        "-n: Notarize macOS app and disk image (implies -b)\n" \
        "-s: Skip build process (useful for bundling/packaging only)\n" \
        "-h: Print this help"
    exit 0
}

obs-build-main() {
    ensure_dir ${CHECKOUT_DIR}
    git fetch origin --tags
    GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    GIT_HASH=$(git rev-parse --short HEAD)
    GIT_TAG=$(git describe --tags --abbrev=0)
    FILE_NAME="obs-studio-${GIT_TAG}-${GIT_HASH}-macOS.dmg"

    ##########################################################################
    # IMPORTANT:
    #
    # Be careful when choosing to notarize and code-sign. The script will try
    # to sign any pre-existing bundle but also pre-existing images.
    #
    # This could lead to a package containing a non-signed bundle, which
    # will then fail notarization.
    #
    # To avoid this, run this script with -b -c first, then -p -c or -p -n
    # after to make sure that a code-signed bundle will be packaged.
    #
    ##########################################################################

    while getopts ":hsbnpc" OPTION; do
        case ${OPTION} in
            h) print_usage ;;
            s) SKIP_BUILD=1 ;;
            b) BUNDLE_OBS=1 ;;
            n) CODESIGN_OBS=1; NOTARIZE_OBS=1 ;;
            p) PACKAGE_OBS=1 ;;
            c) CODESIGN_OBS=1 ;;
            \?) ;;
        esac
    done

    full-build-macos
    bundle_macos
    codesign_bundle
    package_macos
    codesign_image
    notarize_macos

    cleanup
}

obs-build-main $*
