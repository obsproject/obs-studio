#!/bin/bash

##############################################################################
# macOS libobs plugin package function
##############################################################################
#
# This script file can be included in build scripts for macOS or run directly
#
##############################################################################

# Halt on errors
set -eE

package_obs() {
    if [ "${CODESIGN}" ]; then
        read_codesign_ident
    fi

    status "Create macOS disk image"
    trap "caught_error 'package app'" ERR

    info "/!\\ CPack will use an AppleScript to create the disk image, this will lead to a Finder window opening to adjust window settings. /!\\"

    ensure_dir "${CHECKOUT_DIR}"

    step "Package OBS..."
    BUILD_DIR="build_${ARCH}"

    pushd "${BUILD_DIR}" > /dev/null > /dev/null
    cpack -C ${BUILD_CONFIG:-RelWithDebInfo}
    popd > /dev/null

    DMG_NAME=$(/usr/bin/find "${BUILD_DIR}" -type f -name "obs-studio-*.dmg" -depth 1 | sort -rn | head -1)

    if [ "${DMG_NAME}" ]; then
        mv "${DMG_NAME}" "${BUILD_DIR}/${FILE_NAME}"

        step "Codesign OBS disk image..."
        /usr/bin/codesign --force --sign "${CODESIGN_IDENT:--}" "${BUILD_DIR}/${FILE_NAME}"
    else
        error "ERROR No suitable OBS disk image generated"
    fi
}

notarize_obs() {
    status "Notarize OBS"
    trap "caught_error 'notarizing app'" ERR

    if ! exists brew; then
        error "ERROR Homebrew not found - please install homebrew (https://brew.sh)"
        exit 1
    fi

    ensure_dir "${CHECKOUT_DIR}"

    if [ "${NOTARIZE_IMAGE}" ]; then
        trap "_caught_error_hdiutil_verify '${NOTARIZE_IMAGE}'" ERR

        step "Verify OBS disk image ${NOTARIZE_IMAGE}..."
        hdiutil verify "${NOTARIZE_IMAGE}"

        NOTARIZE_TARGET="${NOTARIZE_IMAGE}"
    elif [ "${NOTARIZE_BUNDLE}" ]; then
        NOTARIZE_TARGET="${NOTARIZE_BUNDLE}"
    else
        OBS_IMAGE="${BUILD_DIR}/${FILE_NAME}"

        if [ -f "${OBS_IMAGE}" ]; then
            NOTARIZE_TARGET="${OBS_IMAGE}"
        else
            error "No notarization application bundle ('OBS.app') or disk image ('${NOTARIZE_IMAGE:-${FILE_NAME}}') found"
            return
        fi
    fi

    if [ "$?" -eq 0 ]; then
        read_codesign_ident
        read_codesign_pass

        step "Notarize ${NOTARIZE_TARGET}..."
        /usr/bin/xcrun notarytool submit "${NOTARIZE_TARGET}" --keychain-profile "OBS-Codesign-Password" --wait

        step "Staple the ticket to ${NOTARIZE_TARGET}..."
        /usr/bin/xcrun stapler staple "${NOTARIZE_TARGET}"
    fi
}

_caught_error_hdiutil_verify() {
    error "ERROR during verifying image '${1}'"

    cleanup
    exit 1
}

package-obs-standalone() {
    PRODUCT_NAME="OBS-Studio"

    CHECKOUT_DIR="$(/usr/bin/git rev-parse --show-toplevel)"
    DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
    source "${CHECKOUT_DIR}/CI/include/build_support.sh"
    source "${CHECKOUT_DIR}/CI/include/build_support_macos.sh"

    check_archs
    check_macos_version

    if [ -z "${CI}" ]; then
        step "Fetch OBS tags..."
        /usr/bin/git fetch --tags origin
    fi

    GIT_BRANCH=$(/usr/bin/git rev-parse --abbrev-ref HEAD)
    GIT_HASH=$(/usr/bin/git rev-parse --short=9 HEAD)
    GIT_TAG=$(/usr/bin/git describe --tags --abbrev=0)

    if [ "${BUILD_FOR_DISTRIBUTION}" ]; then
        VERSION_STRING="${GIT_TAG}"
    else
        VERSION_STRING="${GIT_TAG}-${GIT_HASH}"
    fi

    if [ -z "${NOTARIZE_IMAGE}" -a -z "${NOTARIZE_BUNDLE}" ]; then
        if [ "${ARCH}" = "arm64" ]; then
            FILE_NAME="obs-studio-${VERSION_STRING}-macos-arm64.dmg"
        elif [ "${ARCH}" = "universal" ]; then
            FILE_NAME="obs-studio-${VERSION_STRING}-macos.dmg"
        else
            FILE_NAME="obs-studio-${VERSION_STRING}-macos-x86_64.dmg"
        fi

        package_obs
    fi

    if [ "${NOTARIZE}" ]; then
        notarize_obs
    fi
}

print_usage() {
    echo -e "Usage: ${0}\n" \
            "-h, --help                     : Print this help\n" \
            "-q, --quiet                    : Suppress most build process output\n" \
            "-v, --verbose                  : Enable more verbose build process output\n" \
            "-a, --architecture             : Specify build architecture (default: x86_64, alternative: arm64)\n" \
            "-c, --codesign                 : Codesign OBS and all libraries (default: ad-hoc only)\n" \
            "-n, --notarize                 : Notarize OBS (default: off)\n" \
            "--notarize-image [IMAGE]       : Specify existing OBS disk image for notarization\n" \
            "--notarize-bundle [BUNDLE]     : Specify existing OBS application bundle for notarization\n" \
            "--build-dir                    : Specify alternative build directory (default: build)\n"
}

package-obs-main() {
    if [ -z "${_RUN_OBS_BUILD_SCRIPT}" ]; then
        while true; do
            case "${1}" in
                -h | --help ) print_usage; exit 0 ;;
                -q | --quiet ) export QUIET=TRUE; shift ;;
                -v | --verbose ) export VERBOSE=TRUE; shift ;;
                -a | --architecture ) ARCH="${2}"; shift 2 ;;
                -c | --codesign ) CODESIGN=TRUE; shift ;;
                -n | --notarize ) NOTARIZE=TRUE; CODESIGN=TRUE; shift ;;
                --build-dir ) BUILD_DIR="${2}"; shift 2 ;;
                --notarize-image ) NOTARIZE_IMAGE="${2}"; NOTARIZE=TRUE; CODESIGN=TRUE; shift 2 ;;
                --notarize-bundle ) NOTARIZE_BUNDLE="${2}"; NOTARIZE=TRUE; CODESIGN=TRUE; shift 2 ;;
                -- ) shift; break ;;
                * ) break ;;
            esac
        done

        package-obs-standalone
    fi
}

package-obs-main $*
