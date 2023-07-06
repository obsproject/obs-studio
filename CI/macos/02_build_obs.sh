#!/bin/bash

##############################################################################
# macOS build function
##############################################################################
#
# This script file can be included in build scripts for macOS or run directly
#
##############################################################################

# Halt on errors
set -eE

build_obs() {
    status "Build OBS"
    trap "caught_error 'build app'" ERR

    step "Configure OBS..."
    _configure_obs

    ensure_dir "${CHECKOUT_DIR}/"
    step "Build OBS targets..."

    export NSUnbufferedIO=YES

    : "${PACKAGE:=}"
    case "${GITHUB_EVENT_NAME}" in
          push) if [[ ${GITHUB_REF_NAME} =~ [0-9]+.[0-9]+.[0-9]+(-(rc|beta).+)? ]]; then PACKAGE=1; fi ;;
          pull_request) PACKAGE=1 ;;
      esac

    pushd "build_macos" > /dev/null

    if [[ "${PACKAGE}" && "${CODESIGN_IDENT:--}" != '-' ]]; then
        set -o pipefail && xcodebuild ONLY_ACTIVE_ARCH=NO -archivePath "obs-studio.xcarchive" -scheme obs-studio -destination "generic/platform=macOS,name=Any Mac" -parallelizeTargets -hideShellScriptEnvironment archive 2>&1 | xcbeautify
        set -o pipefail && xcodebuild -exportArchive -archivePath "obs-studio.xcarchive" -exportOptionsPlist "exportOptions.plist" -exportPath "." 2>&1 | xcbeautify
    else
        set -o pipefail && xcodebuild ONLY_ACTIVE_ARCH=NO -project obs-studio.xcodeproj -target obs-studio -destination "generic/platform=macOS,name=Any Mac" -parallelizeTargets -configuration RelWithDebInfo -hideShellScriptEnvironment build 2>&1 | xcbeautify

        rm -rf OBS.app && mkdir OBS.app
        ditto UI/RelWithDebInfo/OBS.app OBS.app
    fi

    popd > /dev/null

    unset NSUnbufferedIO
}

bundle_obs() {
    status "Create relocatable macOS application bundle"
    trap "caught_error 'package app'" ERR

    ensure_dir "${CHECKOUT_DIR}"

    step "Install OBS application bundle..."

    find "build_macos/UI/${BUILD_CONFIG}" -type d -name "OBS.app" | xargs -I{} cp -r {} "build_${ARCH}"/
}

# Function to configure OBS build
_configure_obs() {
    if [ "${CODESIGN}" ]; then
        read_codesign_ident
    fi

    ensure_dir "${CHECKOUT_DIR}"
    status "Configure OBS build system..."
    trap "caught_error 'configure build'" ERR
    check_ccache

    if [ "${SPARKLE_APPCAST_URL}" -a "${SPARKLE_PUBLIC_KEY}" ]; then
        SPARKLE_OPTIONS="-DSPARKLE_APPCAST_URL=\"${SPARKLE_APPCAST_URL}\" -DSPARKLE_PUBLIC_KEY=\"${SPARKLE_PUBLIC_KEY}\""
    fi

    PRESET="macos"

    if [ "${CI}" ]; then
        case "${GITHUB_EVENT_NAME}" in
            push)
                if [ "${GITHUB_REF_TYPE}" != 'tag' ]; then
                    PRESET="macos-ci"
                fi
                ;;
            *) PRESET="macos-ci" ;;
        esac
    fi

    cmake -S . --preset ${PRESET} \
        -DCMAKE_OSX_ARCHITECTURES=${ARCH} \
        -DCMAKE_INSTALL_PREFIX=${BUILD_DIR}/install \
        -DCMAKE_BUILD_TYPE=${BUILD_CONFIG} \
        -DOBS_CODESIGN_IDENTITY="${CODESIGN_IDENT:--}" \
        -DOBS_CODESIGN_TEAM="${CODESIGN_TEAM:-}" \
        -DOBS_PROVISIONING_PROFILE="${PROVISIONING_PROFILE:-}" \
        ${YOUTUBE_OPTIONS} \
        ${TWITCH_OPTIONS} \
        ${RESTREAM_OPTIONS} \
        ${SPARKLE_OPTIONS} \
        ${QUIET:+-Wno-deprecated -Wno-dev --log-level=ERROR}
}

build-obs-standalone() {
    CHECKOUT_DIR="$(/usr/bin/git rev-parse --show-toplevel)"
    PRODUCT_NAME="OBS-Studio"
    DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
    source "${CHECKOUT_DIR}/CI/include/build_support.sh"
    source "${CHECKOUT_DIR}/CI/include/build_support_macos.sh"

    check_archs
    check_macos_version
    build_obs

    if [ "${BUNDLE}" ]; then
        bundle_obs
    fi
}

print_usage() {
    echo -e "Usage: ${0}\n" \
            "-h, --help                     : Print this help\n" \
            "-q, --quiet                    : Suppress most build process output\n" \
            "-v, --verbose                  : Enable more verbose build process output\n" \
            "-a, --architecture             : Specify build architecture (default: x86_64, alternative: arm64)\n" \
            "-c, --codesign                 : Codesign OBS and all libraries (default: ad-hoc only)\n" \
            "-b, --bundle                   : Create relocatable OBS application bundle in build directory (default: build/install/OBS.app)\n"
}

build-obs-main() {
    if [ -z "${_RUN_OBS_BUILD_SCRIPT}" ]; then
        while true; do
            case "${1}" in
                -h | --help ) print_usage; exit 0 ;;
                -q | --quiet ) export QUIET=TRUE; shift ;;
                -v | --verbose ) export VERBOSE=TRUE; shift ;;
                -a | --architecture ) ARCH="${2}"; shift 2 ;;
                -c | --codesign ) CODESIGN=TRUE; shift ;;
                -b | --bundle ) BUNDLE=TRUE; shift ;;
                -- ) shift; break ;;
                * ) break ;;
            esac
        done

        build-obs-standalone
    fi
}

build-obs-main $*
