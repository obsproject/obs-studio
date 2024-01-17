#!/bin/zsh

# 1. Put the script to an empty folder
# 2. ./create-webrtc-package.zsh --architecture=arm64
# 3. ./create-webrtc-package.zsh --architecture=x86_64

# If you have the following error during gclient sync:
#   Error: Command 'python3 src/tools/clang/scripts/update.py' returned non-zero exit status 1 in /Users/eugen/devel/sl-webrtc-1/webrtc
#   Downloading https://commondatastorage.googleapis.com/chromium-browser-clang/Mac_arm64/clang-llvmorg-16-init-17653-g39da55e8-2.tar.xz 
#   <urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:992)>
#   Retrying in 5 s ...
# Update Python 3 certificates. Just browse to Applications/Python 3.6 and double-click Install Certificates.command
# if you installed Python 3 via the official installer.

# if you have the following error during gclient sync:
# 0> Downloading src/third_party/test_fonts/test_fonts.tar.gz@336e775eec536b2d785cc80eff6ac39051931286...
# 0> Failed to fetch file gs://chromium-fonts/336e775eec536b2d785cc80eff6ac39051931286 for src/third_party/test_fonts/test_fonts.tar.gz, skipping. [Err: ]
# 1. Check that your python/python3 is not outdated.
#    Some google answers will recommend you to remove access to the system python binaries and force depot_tools/python-bin/python3
#    That did not help me.
# 2. After debugging download_from_google_storage.py I saw this warning: 
#    "Using luci-auth login since OOB is deprecated.
#    Override luci-auth by setting `BOTO_CONFIG` in your env.
#    Not logged in.
#    Login by running:
#    $ gsutil.py config"
#    So, run that command. It will show you some instructions and you have to login with you gmail/company account to chromium.org.
# 3. Unfortunately, 2nd step did not help me enough. It is possible you have go to https://cloud.google.com/console#/project
#    to accept the license. Then, run "gsutil.py config" again.
# It looks like this feature is being developed right now. So the solution may be easier or more complicated in the future.

# If you have the following error during compilation:
#   NSString.h:371:81: error: function does not return string type
# or other compilation erros on macOS Ventura and later/Xcode 14,
# you have to downgrade to an older XCode version (for me Xcode 13 worked).
# Download it from here: https://developer.apple.com/download/more/
# Unpack it and copy to the /Application folder, then run sudo xcode-select --switch /Applications/Xcode-13.4.app/

# if you have the following error during compilation:
#   FAILED: nasm 
#   TOOL_VERSION=1674864830 ../../build/toolchain/apple/linker_driver.py ...
#   env: python: No such file or directory
# then the solution should be something like this: sudo ln -sf /usr/bin/python3 /usr/local/bin/python
# Unfortunately, that did not help me. Xcode started to ask me to install command line tools repeatedly.
# Even using python3 from an Xcode subfolder did not help.
# So, do the following:
# - Delete the webrtc-checkout/src/out folder.
# - Edit webrtc-checkout/src/build/toolchain/apple/toolchain.gni by changing
#     linker_driver =
#        "TOOL_VERSION=${tool_versions.linker_driver} " +
#        rebase_path("//build/toolchain/apple/linker_driver.py", root_build_dir)
#   to
#     linker_driver =
#        "TOOL_VERSION=${tool_versions.linker_driver} python3 " +
#        rebase_path("//build/toolchain/apple/linker_driver.py", root_build_dir)

install_depot_tools() {
    # mkdir depot_tools
    # cd depot_tools
    # git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    # export PATH=$PWD:$PATH
    # cd ..

    # Check if the folder exists
    if [ -d "depot_tools" ]
    then
        echo "### The 'depot_tools' folder exists. Please delete it or install 'depot_tools' manually and add to the PATH, then start the script again."
        exit 1
    fi
    # Clone
    echo "### git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git"
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
    # Check if failed
    if [ $? -ne 0 ]
    then
        echo "### Could not install 'depot_tools'. Please install it manually and add to the PATH, then start the script again."
        exit 1
    fi
    # Add to PATH
    export PATH=${PWD}/depot_tools:$PATH
}

download_webrtc() {
    # mkdir webrtc
    # cd webrtc
    # fetch --nohooks webrtc
    # gclient sync
    # cd src
    # git checkout -b m94 refs/remotes/branch-heads/4606
    # gclient sync

    if [ -d "${SRC_PARENT_FOLDER}" ]
    then
        echo "### The sources will not be downloaded because '${SRC_PARENT_FOLDER}' exists. Remove it manually if you want to re-download the sources!"
        if [ ! -d "${GIT_FOLDER}" ]
        then
            echo "### '${GIT_FOLDER}' does not exist. The sources are broken. Remove '${SRC_PARENT_FOLDER}' and restart the script."
            exit 1
        fi
        return
    fi

    # Create the source parent folder
    mkdir "${SRC_PARENT_FOLDER}"
    cd "${SRC_PARENT_FOLDER}"

    # Fetch the source code
    echo "### fetch --nohooks webrtc"
    fetch --nohooks webrtc
    if [ $? -ne 0 ]
    then
        echo "### Could not fetch the webrtc source code."
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    # Check if the source folder name is correct and the folder exists
    if [ ! -d "${GIT_FOLDER}" ]
    then
        echo "### The source folder '${GIT_FOLDER}' could not be found. Probably the 'fetch' command failed. Please check manually"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    # Sync
    echo "### gclient sync"
    gclient sync
    if [ $? -ne 0 ]
    then
        echo "Could not sync the webrtc source code."
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    # Get the appropriate version
    cd "${GIT_FOLDER}"
    echo "### git checkout -b ${GIT_BRANCH_NAME} ${GIT_REFS}"
    git checkout -b ${VERSION_NAME} ${GIT_REFS}
    if [ $? -ne 0 ]
    then
        echo "### Could not checkout the appropriate webrtc version."
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    # Sync
    echo "### gclient sync -D"
    gclient sync -D
    if [ $? -ne 0 ]
    then
        echo "### Could not sync the webrtc source code."
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi
}

patch_webrtc() {
    BUILD_GN=${GIT_FOLDER}/build/config/mac/BUILD.gn

    if grep -q "NS_FORMAT_ARGUMENT" "${BUILD_GN}"
    then
        echo "### It looks like '${GIT_FOLDER}/build/config/mac/BUILD.gn' does not need to be patched!"
    else
        echo "### Patching '${GIT_FOLDER}/build/config/mac/BUILD.gn' ..."

        PATCH_FILENAME=build-gn-fixes.patch
        PATCH_PATH=${GIT_FOLDER}/${PATCH_FILENAME}

        cat <<'EOT' > "${PATCH_PATH}"
--- a/build/config/mac/BUILD.gn	2024-01-14 22:05:29
+++ b/build/config/mac/BUILD.gn	2024-01-15 22:19:30
@@ -68,7 +68,9 @@
 
   asmflags = common_flags
   cflags = common_flags
+  cflags += [ "-DNS_FORMAT_ARGUMENT(A)=", "-Wno-c++11-narrowing" ]
   ldflags = common_flags
+  ldflags += [ "-Wl,-ld_classic" ]
 
   # Prevent Mac OS X AssertMacros.h (included by system header) from defining
   # macros that collide with common names, like 'check', 'require', and
EOT

        if [ $? -ne 0 ]
        then
            echo "### Could not prepare the macos BUILD.gn patch!"
            exit 1
        fi 

        cd "${GIT_FOLDER}"
        git apply "${PATCH_FILENAME}"
        if [ $? -ne 0 ]
        then
            echo "### Could not apply the macos BUILD.gn patch!"
            cd "${INITIAL_WORKING_FOLDER}"
            exit 1
        fi 

        cd "${INITIAL_WORKING_FOLDER}"
    fi
}

build_webrtc() {
    # cd src
    # gn gen out/m94-arm64 --args='target_os="mac" target_cpu="arm64" mac_deployment_target="11.0" mac_min_system_version="11.0" mac_sdk_min="11.0" is_debug=false is_component_build=false is_clang=true rtc_include_tests=false use_rtti=true use_custom_libcxx=false treat_warnings_as_errors=false' --ide=xcode
    # ninja -C out/m94-arm64
    # gn gen out/m94-x86_64 --args='target_os="mac" target_cpu="x64" mac_deployment_target="10.15.0" mac_min_system_version="10.15.0" mac_sdk_min="10.15" is_debug=false is_component_build=false is_clang=true rtc_include_tests=false use_rtti=true use_custom_libcxx=false treat_warnings_as_errors=false' --ide=xcode
    # ninja -C out/m94-x86_64

    # Prepare build parameters
    case "${ARCHITECTURE}" in
        arm64)    
            PARAMS="target_os=\"mac\" target_cpu=\"arm64\" mac_deployment_target=\"11.0\" mac_min_system_version=\"11.0\" mac_sdk_min=\"11.0\""
            ;;
        x86_64)
            PARAMS="target_os=\"mac\" target_cpu=\"x64\" mac_deployment_target=\"10.15.0\" mac_min_system_version=\"10.15.0\" mac_sdk_min=\"10.15\""
            ;;
        *)
            echo "Unknown architecture! Only 'arm64' or 'x86_64' is supported."
            exit 1
            ;;
    esac

    cd "${GIT_FOLDER}"

    # Check if the folder exists
    if [ -d "${BUILD_FOLDER_REL}" ]
    then
        echo "### The build folder '${BUILD_FOLDER}' exits. It will not be reconfigured. Remove it manually if you want to configure it from scratch."
    else
        # Configure
        echo "### gn gen ${BUILD_FOLDER_REL} ..."
        gn gen ${BUILD_FOLDER_REL} --args="${PARAMS} is_debug=false is_component_build=false is_clang=true rtc_include_tests=false use_rtti=true use_custom_libcxx=false treat_warnings_as_errors=false" --ide=xcode
        if [ $? -ne 0 ]
        then
            echo "### Could not configure to build for ${ARCHITECTURE}"
            cd "${INITIAL_WORKING_FOLDER}"
            exit 1
        fi    
    fi

    # Build
    echo "### ninja -C ${BUILD_FOLDER_REL} ..."
    ninja -C ${BUILD_FOLDER_REL}
    if [ $? -ne 0 ]
    then
        echo "### Could not build for ${ARCHITECTURE}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    cd "${INITIAL_WORKING_FOLDER}"
}

package_webrtc() {
    PACKAGE_FOLDER_NAME=webrtc-dist-osx-${VERSION_NAME}-b$(date +'%y%m%d')-${ARCHITECTURE}
    PACKAGE_FOLDER=${INITIAL_WORKING_FOLDER}/${PACKAGE_FOLDER_NAME}
    if [ -d "$PACKAGE_FOLDER" ]
    then
        echo "### Package folder ${PACKAGE_FOLDER} exists. Remove it manually and start the script again."
        exit 1    
    fi
    if [ -e "$PACKAGE_FOLDER.zip" ]
    then
        echo "### The package ${PACKAGE_FOLDER}.zip exists. Remove it manually and start the script again."
        exit 1    
    fi   
    echo "### Creating the package folder: ${PACKAGE_FOLDER}"
    # Create the package root folder
    mkdir -p "${PACKAGE_FOLDER}"
    # Copy libs
    echo "### Copying libraries ..."
    cp "${BUILD_FOLDER}/obj/libwebrtc.a" "${PACKAGE_FOLDER}"
    # Copy includes
    echo "### Copying includes ..."
    cd "${GIT_FOLDER}"
    find . -name '*.h' -not -path "./out/*" | cpio -pdm "${PACKAGE_FOLDER}"
    echo "### Copying some sources ..."
    # We also need some *.cc; if you like, just include all ccs
    #find . -name '*.cc' -not -path "./out/*" | cpio -pdm "${PACKAGE_FOLDER}"
    # Here I am trying to reduce the size of the package by including only what is necessary.
    cd "${GIT_FOLDER}/test"
    find . -name '*.cc' | cpio -pdm "${PACKAGE_FOLDER}/test"    
    cd "${GIT_FOLDER}/api/test"
    find . -name '*.cc' | cpio -pdm "${PACKAGE_FOLDER}/api/test"
    cd "${GIT_FOLDER}/media/base"
    find . -name '*.cc' | cpio -pdm "${PACKAGE_FOLDER}/media/base"
    cd "${GIT_FOLDER}/pc/test"
    find . -name '*.cc' | cpio -pdm "${PACKAGE_FOLDER}/pc/test"
    cd "${GIT_FOLDER}/rtc_base"
    find . -name '*.cc' | cpio -pdm "${PACKAGE_FOLDER}/rtc_base"

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
        *)
            echo "Error: Invalid command line parameter. Please use --architecture=\"arm64\" or --architecture=\"x86_64\"."
            exit 1
    esac
    shift
done

INITIAL_WORKING_FOLDER=${PWD}
SCRIPT_PATH=$(realpath $0)
SRC_PARENT_FOLDER_NAME=webrtc-checkout
SRC_PARENT_FOLDER=${PWD}/${SRC_PARENT_FOLDER_NAME}
GIT_FOLDER_NAME=src
GIT_FOLDER=${SRC_PARENT_FOLDER}/${GIT_FOLDER_NAME}
GIT_REFS=refs/remotes/branch-heads/4606
VERSION_NAME=m94
BUILD_FOLDER_NAME=${VERSION_NAME}-${ARCHITECTURE}
BUILD_FOLDER_REL=out/${BUILD_FOLDER_NAME}
BUILD_FOLDER=${GIT_FOLDER}/${BUILD_FOLDER_REL}

# Check if git is available
if ! command -v git &> /dev/null
then
    echo "'git' could not be found. Please install 'git' and start the script again."
    exit 1
fi

# Check if depot_tools are available
if ! command -v gclient &> /dev/null
then
    # Check the subfolder
    export PATH=${PWD}/depot_tools:$PATH
    if ! command -v gclient &> /dev/null
    then
        echo "Could not find 'depot_tools'. Installing..."
        install_depot_tools
    fi
fi

# Downlaod the source code if it is necessary
download_webrtc

# Patch build/config/mac/BUILD.gn to build it with the latest XCode
patch_webrtc

# Build 
build_webrtc

# Package
package_webrtc

