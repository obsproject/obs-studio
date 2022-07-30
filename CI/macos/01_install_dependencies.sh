#!/bin/bash

##############################################################################
# macOS dependency management function
##############################################################################
#
# This script file can be included in build scripts for macOS or run directly.
#
##############################################################################

# Halt on errors
set -eE

install_obs-deps() {
    status "Set up precompiled macOS OBS dependencies v${1}"
    ensure_dir "${DEPS_BUILD_DIR}"
    step "Download..."
    check_and_fetch "https://github.com/obsproject/obs-deps/releases/download/${1}/macos-deps-${1}-${ARCH:-x86_64}.tar.xz" "${2}"
    mkdir -p obs-deps
    step "Unpack..."
    /usr/bin/tar -xf "./macos-deps-${1}-${ARCH:-x86_64}.tar.xz" -C ./obs-deps
    /usr/bin/xattr -r -d com.apple.quarantine ./obs-deps
}

install_qt-deps() {
    status "Set up precompiled dependency Qt v${1}"
    ensure_dir "${DEPS_BUILD_DIR}"
    step "Download..."

    if [[ -n ${CI} ]]; then
        _ARCH='universal'
        _HASH=${CI_QT_HASH_UNIVERSAL}
    else
        _ARCH="${ARCH:-x86_64}"
        _HASH="${2}"
    fi

    check_and_fetch "https://github.com/obsproject/obs-deps/releases/download/${1}/macos-deps-qt5-${1}-${_ARCH}.tar.xz" "${_HASH}"
    mkdir -p obs-deps
    step "Unpack..."
    /usr/bin/tar -xf "./macos-deps-qt5-${1}-${_ARCH}.tar.xz" -C ./obs-deps
    /usr/bin/xattr -r -d com.apple.quarantine ./obs-deps
}

install_vlc() {
    status "Set up dependency VLC v${1}"
    ensure_dir "${DEPS_BUILD_DIR}"
    unset _SKIP

    if [ "${CI}" -a "${RESTORED_VLC}" ]; then
        _SKIP=TRUE
    elif [ -d "${DEPS_BUILD_DIR}/vlc-${1}" -a -f "${DEPS_BUILD_DIR}/vlc-${1}/include/vlc/vlc.h" ]; then
        _SKIP=TRUE
    fi

    if [ -z "${_SKIP}" ]; then
        step "Download..."
        check_and_fetch "https://downloads.videolan.org/vlc/${1}/vlc-${1}.tar.xz" "${2}"
        step "Unpack..."
        /usr/bin/tar -xf vlc-${1}.tar.xz
    else
        step "Found existing VLC..."
    fi
}

install_sparkle() {
    status "Set up dependency Sparkle v${1}"
    ensure_dir "${DEPS_BUILD_DIR}"
    unset _SKIP

    if [ "${CI}" -a "${RESTORED_SPARKLE}" ]; then
        _SKIP=TRUE
    elif [ -d "${DEPS_BUILD_DIR}/obs-deps/lib/Sparkle.framework" -a -f "${DEPS_BUILD_DIR}/obs-deps/lib/Sparkle.framework/Sparkle" ]; then
        _SKIP=TRUE
    fi

    if [ -z "${_SKIP}" ]; then
        step "Download..."
        check_and_fetch "https://github.com/sparkle-project/Sparkle/releases/download/${1}/Sparkle-${1}.tar.xz" "${2}"
        step "Unpack..."
        ensure_dir "${DEPS_BUILD_DIR}/sparkle"
        /usr/bin/tar -xf ../Sparkle-${1}.tar.xz
        cp -cpR "${DEPS_BUILD_DIR}"/sparkle/Sparkle.framework "${DEPS_BUILD_DIR}"/obs-deps/lib/
    else
        step "Found existing Sparkle Framework..."
    fi
}

install_cef() {
    status "Set up dependency CEF v${1}"
    ensure_dir "${DEPS_BUILD_DIR}"
    unset _SKIP
    
    if [ "${CI}" -a "${RESTORED_CEF}" ]; then
        _SKIP=TRUE
    elif [ -d "${DEPS_BUILD_DIR}/cef_binary_${1}_macos_${ARCH:-x86_64}" -a -f "${DEPS_BUILD_DIR}/cef_binary_${1}_macos_${ARCH:-x86_64}/build/libcef_dll_wrapper/libcef_dll_wrapper.a" ]; then
        _SKIP=TRUE
    fi

    if [ -z "${_SKIP}" ]; then
        step "Download..."
        check_and_fetch "https://cdn-fastly.obsproject.com/downloads/cef_binary_${1}_macos_${ARCH:-x86_64}.tar.xz" "${2}"
        step "Unpack..."
        /usr/bin/tar -xf cef_binary_${1}_macos_${ARCH:-x86_64}.tar.xz
        cd cef_binary_${1}_macos_${ARCH:-x86_64}
        step "Fix tests..."

        /usr/bin/sed -i '.orig' '/add_subdirectory(tests\/ceftests)/d' ./CMakeLists.txt
        /usr/bin/sed -E -i '' 's/"10.(9|10|11)"/"'${MACOSX_DEPLOYMENT_TARGET:-${CI_MACOSX_DEPLOYMENT_TARGET}}'"/' ./cmake/cef_variables.cmake

        step "Run CMake..."
        check_ccache
        cmake ${CCACHE_OPTIONS} ${QUIET:+-Wno-deprecated -Wno-dev --log-level=ERROR} \
            -S . -B build \
            -G Ninja \
            -DPROJECT_ARCH=${CMAKE_ARCHS:-x86_64} \
            -DCEF_COMPILER_FLAGS="-Wno-deprecated-copy" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++ -Wno-deprecated-declarations -Wno-unknown-warning-option" \
            -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-${CI_MACOSX_DEPLOYMENT_TARGET}}

        step "Build CEF v${1}..."
        cmake --build build
        mkdir -p build/libcef_dll
    else
        step "Found existing Chromium Embedded Framework and loader library..."
    fi
}

install_dependencies() {
    status "Install Homebrew dependencies"
    trap "caught_error 'install_dependencies'" ERR

    BUILD_DEPS=(
        "obs-deps ${MACOS_DEPS_VERSION:-${CI_DEPS_VERSION}} ${MACOS_DEPS_HASH:-${CI_DEPS_HASH}}"
        "qt-deps ${MACOS_DEPS_VERSION:-${CI_DEPS_VERSION}} ${QT_HASH:-${CI_QT_HASH}}"
        "cef ${MACOS_CEF_BUILD_VERSION:-${CI_MACOS_CEF_VERSION}} ${CEF_HASH:-${CI_CEF_HASH}}"
        "vlc ${VLC_VERSION:-${CI_VLC_VERSION}} ${VLC_HASH:-${CI_VLC_HASH}}"
        "sparkle ${SPARKLE_VERSION:-${CI_SPARKLE_VERSION}} ${SPARKLE_HASH:-${CI_SPARKLE_HASH}}"
    )

    install_homebrew_deps

    for DEPENDENCY in "${BUILD_DEPS[@]}"; do
        set -- ${DEPENDENCY}
        trap "caught_error ${DEPENDENCY}" ERR
        FUNC_NAME="install_${1}"
        ${FUNC_NAME} ${2} ${3} ${4}
    done
}

install-dependencies-standalone() {
    CHECKOUT_DIR="$(/usr/bin/git rev-parse --show-toplevel)"
    PRODUCT_NAME="OBS-Studio"
    DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
    source "${CHECKOUT_DIR}/CI/include/build_support.sh"
    source "${CHECKOUT_DIR}/CI/include/build_support_macos.sh"

    status "Setup of OBS build dependencies"
    check_macos_version
    check_archs
    install_dependencies
}

print_usage() {
    echo -e "Usage: ${0}\n" \
            "-h, --help                     : Print this help\n" \
            "-q, --quiet                    : Suppress most build process output\n" \
            "-v, --verbose                  : Enable more verbose build process output\n" \
            "-a, --architecture             : Specify build architecture (default: x86_64, alternative: arm64)\n"
}

install-dependencies-main() {
    if [ -z "${_RUN_OBS_BUILD_SCRIPT}" ]; then
        while true; do
            case "${1}" in
                -h | --help ) print_usage; exit 0 ;;
                -q | --quiet ) export QUIET=TRUE; shift ;;
                -v | --verbose ) export VERBOSE=TRUE; shift ;;
                -a | --architecture ) ARCH="${2}"; shift 2 ;;
                -- ) shift; break ;;
                * ) break ;;
            esac
        done

        install-dependencies-standalone
    fi
}

install-dependencies-main $*
