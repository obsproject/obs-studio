#!/bin/zsh

# 1. Precompile the webrtc library for arm64 or/and x84_64
# 2. Place the script to an empty folder
# 3. ./create-libmediasoupclient-package.zsh --architecture=arm64 --webrtc-include-folder=...  --webrtc-lib-folder=ARM64_OR_UNIVERSAL_LIB_FOLDER
# 4. ./create-libmediasoupclient-package.zsh --architecture=x86_64 --webrtc-include-folder=...  --webrtc-lib-folder=X86_64_OR_UNIVERSAL_LIB_FOLDER

download_libmediasoupclient() {    
    # Check if the webrtc folder exists
    if [ -d "${GIT_FOLDER}" ]
    then
        echo "### The '${GIT_FOLDER}' folder exists. It will be reused. Remove it if you want to clone the git repositoty again."
        return
    fi

    # Clone
    git clone --recurse-submodules https://github.com/versatica/libmediasoupclient.git ${GIT_FOLDER}
    if [ $? -ne 0 ]
    then
        echo "### ould not clone libmediasoupclient."
        exit 1
    fi

    # Check if the source folder name is correct and the folder exists
    if [ ! -d "${GIT_FOLDER}" ]
    then
        echo "### The source folder '${GIT_FOLDER}' could not be found. Probably the 'git clone' command failed. Please check manually"
        exit 1
    fi
}

build_libmediasoupclient() {
    # Prepare build parameters
    case "${ARCHITECTURE}" in
        arm64)
            MIN_MACOS_VERSION=11.0
            ;;
        x86_64)
            MIN_MACOS_VERSION=10.15
            ;;
        *)
            echo "### Unknown architecture! Only 'arm64' or 'x86_64' is supported."
            exit 1
            ;;
    esac

    # Check if the build folder exists
    if [ -d "${BUILD_FOLDER}" ]
    then
        echo "### The '${BUILD_FOLDER}' folder exists. Configuring will be skiped. Remove the folder if you want to configure from scratch."
        cd "${BUILD_FOLDER}"
    else
        mkdir -p "${BUILD_FOLDER}"
        if [ $? -ne 0 ]
        then
            echo "### Could not create the build folder: ${BUILD_FOLDER}"
            exit 1
        fi
        cd "${BUILD_FOLDER}"
        cmake .. -DLIBWEBRTC_INCLUDE_PATH="${WEBRTC_INCLUDE_FOLDER}" -DLIBWEBRTC_BINARY_PATH="${WEBRTC_LIB_FOLDER}" -DCMAKE_OSX_ARCHITECTURES="${ARCHITECTURE}" -DCMAKE_OSX_DEPLOYMENT_TARGET=${MIN_MACOS_VERSION}
        if [ $? -ne 0 ]
        then
            echo "### Could not configure to build for ${ARCHITECTURE}"
            cd "${INITIAL_WORKING_FOLDER}"
            exit 1
        fi
    fi

    cmake --build .
    if [ $? -ne 0 ]
    then
        echo "### Build failed for ${ARCHITECTURE}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi    
}

package_libmediasoupclient() {
    PACKAGE_FOLDER_NAME=libmediasoupclient-dist-osx-$(git -C ${GIT_FOLDER} rev-parse --short HEAD)-b$(date +'%y%m%d')-${ARCHITECTURE}
    PACKAGE_FOLDER=${INITIAL_WORKING_FOLDER}/${PACKAGE_FOLDER_NAME}

    if [ -d "${PACKAGE_FOLDER}" ]; then
        echo "### Package folder ${PACKAGE_FOLDER} exists. Plrease remove it manually and start the script again."
        exit 1    
    fi

    # Create the folder
    echo "### Creating the package folder: ${PACKAGE_FOLDER}"
    mkdir -p "${PACKAGE_FOLDER}/include/mediasoupclient"
    mkdir -p "${PACKAGE_FOLDER}/include/sdptransform"
    mkdir -p "${PACKAGE_FOLDER}/lib"
    
    # Copy libs
    echo "### Copying libraries ..."
    cp "${BUILD_FOLDER}/libmediasoupclient.a" "${PACKAGE_FOLDER}/lib"
    cp "${BUILD_FOLDER}/_deps/libsdptransform-build/libsdptransform.a" "${PACKAGE_FOLDER}/lib"

    # Copy includes    
    echo "### Copying includes ..."
    cp -R "${GIT_FOLDER}/include/." "${PACKAGE_FOLDER}/include/mediasoupclient"
    cp -R "${BUILD_FOLDER}/_deps/libsdptransform-src/include/." "${PACKAGE_FOLDER}/include/sdptransform"

    # Copy the script
    cp "${SCRIPT_PATH}" "${PACKAGE_FOLDER}"

    cd ${INITIAL_WORKING_FOLDER}

    # Zip everything
    echo "### Compressing ..."
    zip --quiet --recurse-paths ${PACKAGE_FOLDER_NAME}.zip ${PACKAGE_FOLDER_NAME}
}

# SCRIPT START

ARCHITECTURE=$(uname -m)

while [ $# -gt 0 ]; do
    case "$1" in
        --architecture=*)
            ARCHITECTURE="${1#*=}"
            ;;
        --webrtc-include-folder=*)
            WEBRTC_INCLUDE_FOLDER="${1#*=}"
            ;;
        --webrtc-lib-folder=*)
            WEBRTC_LIB_FOLDER="${1#*=}"
            ;;
        *)
            echo "### Error: Invalid command line parameter. Please use --architecture=arm64|x86_64 --webrtc-include-folder=...  --webrtc-lib-folder=..."
            exit 1
    esac
    shift
done

if [ -z "${WEBRTC_INCLUDE_FOLDER}" ]; then
    echo "### The webrtc include folder is not set. Please specify --webrtc-include-folder=..."
    exit 1
fi

if [ -z "${WEBRTC_LIB_FOLDER}" ]; then
    echo "### The webrtc lib folder is not set. Please specify --webrtc-lib-folder=..."
    exit 1
fi

INITIAL_WORKING_FOLDER=${PWD}
SCRIPT_PATH=$(realpath $0)
GIT_FOLDER_NAME=libmediasoupclient
GIT_FOLDER=${PWD}/${GIT_FOLDER_NAME}
BUILD_FOLDER_NAME=build-${ARCHITECTURE}
BUILD_FOLDER=${GIT_FOLDER}/${BUILD_FOLDER_NAME}

# Check if git is available
if ! command -v git &> /dev/null
then
    echo "'git' could not be found. Please install 'git' and start the script again."
    exit 1
fi

# Download the sources
download_libmediasoupclient

# Build
build_libmediasoupclient

# Package
package_libmediasoupclient
