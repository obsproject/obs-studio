#!/bin/zsh

# 1. Place the script to an empty folder
# 2. ./create-obs-deps-package.zsh --architecture=arm64
# 3. ./create-obs-deps-package.zsh --architecture=x86_64

download_obs_deps() {    
    # Check if the webrtc folder exists
    if [ -d "${GIT_FOLDER}" ]
    then
        echo "### The '${GIT_FOLDER}' folder exists. It will be reused. Remove it if you want to clone the git repositoty again."
        return
    fi

    # Clone
    git clone --recurse-submodules https://github.com/obsproject/obs-deps.git ${GIT_FOLDER}
    if [ $? -ne 0 ]
    then
        echo "### Could not clone!"
        exit 1
    fi

    # Check if the source folder name is correct and the folder exists
    if [ ! -d "${GIT_FOLDER}" ]
    then
        echo "### The source folder '${GIT_FOLDER}' could not be found. Probably the 'git clone' command failed. Please check manually!"
        exit 1
    fi

    cd "${GIT_FOLDER}"
    git checkout -b ${GIT_TAG}-$(date +"%s") ${GIT_TAG}
    if [ $? -ne 0 ]
    then
        echo "### Could not checkout the tag ${GIT_TAG}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    cd "${INITIAL_WORKING_FOLDER}"
}

patch_obs_deps() {
    ONLY_FOR_TAG=2022-08-02
    if [ "${GIT_TAG}" != "${ONLY_FOR_TAG}" ]; then
        echo "### Review patch_obs_deps() function in this script which is written for obs_deps ${ONLY_FOR_TAG}. Modify it to patch obs_deps ${GIT_TAG} correctly!"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1        
    fi

    # Copy the prepared C++ patch first
    CUVIDDEC_C_PATCH=${GIT_FOLDER}/deps.ffmpeg/patches/FFmpeg/0004-FFmpeg-5.0.1-cuvid.patch
    if [ ! -d "${CUVIDDEC_C_PATCH}" ]
    then
        echo "### Saving ${CUVIDDEC_C_PATCH} ..."

        cat <<'EOT' > "${CUVIDDEC_C_PATCH}"
diff --git a/libavcodec/cuviddec.c b/libavcodec/cuviddec.c
index f03bbd8..7b8ec3a 100644
--- a/libavcodec/cuviddec.c
+++ b/libavcodec/cuviddec.c
@@ -817,7 +817,7 @@ static av_cold int cuvid_decode_init(AVCodecContext *avctx)
                                    &ctx->resize.width, &ctx->resize.height) != 2) {
         av_log(avctx, AV_LOG_ERROR, "Invalid resize expressions\n");
         ret = AVERROR(EINVAL);
-        goto error;
+        return ret;
     }
 
     if (ctx->crop_expr && sscanf(ctx->crop_expr, "%dx%dx%dx%d",
@@ -825,13 +825,13 @@ static av_cold int cuvid_decode_init(AVCodecContext *avctx)
                                  &ctx->crop.left, &ctx->crop.right) != 4) {
         av_log(avctx, AV_LOG_ERROR, "Invalid cropping expressions\n");
         ret = AVERROR(EINVAL);
-        goto error;
+        return ret;
     }
 
     ret = cuvid_load_functions(&ctx->cvdl, avctx);
     if (ret < 0) {
         av_log(avctx, AV_LOG_ERROR, "Failed loading nvcuvid.\n");
-        goto error;
+        return ret;
     }
 
     ctx->frame_queue = av_fifo_alloc(ctx->nb_surfaces * sizeof(CuvidParsedFrame));
EOT
        if [ $? -ne 0 ]
        then
            echo "### Could not save patch: ${CUVIDDEC_C_PATCH}"
            cd "${INITIAL_WORKING_FOLDER}"
            exit 1
        fi
    else
        echo "### Found ${CUVIDDEC_C_PATCH}"
    fi

    FFMPEG_ZSH_PATCH=${GIT_FOLDER}/99-ffmpeg-zsh.patch
    FFMPEG_ZSH=${GIT_FOLDER}/deps.ffmpeg/99-ffmpeg.zsh
    if grep -q "0004-FFmpeg-5.0.1-cuvid.patch" "${FFMPEG_ZSH}"
    then
        echo "### It looks like the '${FFMPEG_ZSH} does not need to be patched!"
    else
        echo "### Saving ${FFMPEG_ZSH_PATCH} ..."

        cat <<'EOT' > "${FFMPEG_ZSH_PATCH}"
--- a/deps.ffmpeg/99-ffmpeg.zsh	2023-01-24 20:34:49
+++ b/deps.ffmpeg/99-ffmpeg.zsh	2023-01-24 20:50:51
@@ -12,6 +12,8 @@
     710fb5a381f7b68c95dcdf865af4f3c63a9405c305abef55d24c7ab54e90b182"
   "* ${0:a:h}/patches/FFmpeg/0003-FFmpeg-5.0.1-librist-7f3f3539e8.patch \
     6b5797b7d897d04db5c8d82009a3705c330fc7461676d51712b1012ff0916f0b"
+  "* ${0:a:h}/patches/FFmpeg/0004-FFmpeg-5.0.1-cuvid.patch \
+    d44609a43f7f09819c74cdfa6fa90c9a1de61b3673aa95e87a294c259f203717"
 )
 
 ## Build Steps
@@ -210,6 +212,8 @@
     --disable-outdev=sdl
     --disable-doc
     --disable-postproc
+    --disable-encoder="hevc"
+    --disable-decoder="hevc"
   )
 
   if (( ! shared_libs )) args+=(--pkg-config-flags="--static")
EOT
        if [ $? -ne 0 ]
        then
            echo "### Could not save patch: ${FFMPEG_ZSH_PATCH}"
            cd "${INITIAL_WORKING_FOLDER}"
            exit 1
        fi

        cd "${GIT_FOLDER}"
        echo "### Patching '${FFMPEG_ZSH}' ..."
        git apply "${FFMPEG_ZSH_PATCH}"
        if [ $? -ne 0 ]
        then
            echo "### Could not apply the FFmpeg patch to disable HEVC: ${FFMPEG_ZSH_PATCH}"
            cd "${INITIAL_WORKING_FOLDER}"
            exit 1
        fi 
    fi

    cd "${INITIAL_WORKING_FOLDER}"
}

build_obs_deps() {
    cd ${GIT_FOLDER}
    echo "### ./build-deps.zsh --target macos-${ARCHITECTURE} --config Release --shared"
    ./build-deps.zsh --target macos-${ARCHITECTURE} --config Release --shared
    if [ $? -ne 0 ]
    then
        echo "### ./build-deps.zsh failed for ${ARCHITECTURE}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi
    echo "### ./build-ffmpeg.zsh --target macos-${ARCHITECTURE} --config Release --static"
    ./build-ffmpeg.zsh --target macos-${ARCHITECTURE} --config Release --static
    if [ $? -ne 0 ]
    then
        echo "### ./build-ffmpeg.zsh failed for ${ARCHITECTURE}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi
    cd "${INITIAL_WORKING_FOLDER}"
}

package_obs_deps() {
    PACKAGE_FOLDER_NAME=macos-deps-${GIT_TAG}-b$(date +'%y%m%d')-${ARCHITECTURE}-sl
    PACKAGE_FOLDER=${INITIAL_WORKING_FOLDER}/${PACKAGE_FOLDER_NAME}

    if [ -d "${PACKAGE_FOLDER}" ]; then
        echo "### Package folder ${PACKAGE_FOLDER} exists. Plrease remove it manually and start the script again."
        exit 1    
    fi

    echo "### Copying files for the package ..."

    OBS_DEPS_ARCH_FOLDER=${GIT_FOLDER}/macos-${ARCHITECTURE}/obs-deps-${ARCHITECTURE}/
    cp -R "${OBS_DEPS_ARCH_FOLDER}" "${PACKAGE_FOLDER}"
    if [ $? -ne 0 ]
    then
        echo "### Could not copy the folder: ${OBS_DEPS_ARCH_FOLDER}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    OBS_FFMPEG_ARCH_FOLDER=${GIT_FOLDER}/macos-${ARCHITECTURE}/obs-ffmpeg-${ARCHITECTURE}/
    cp -R "${OBS_FFMPEG_ARCH_FOLDER}" "${PACKAGE_FOLDER}"
    if [ $? -ne 0 ]
    then
        echo "### Could not copy the folder: ${OBS_FFMPEG_ARCH_FOLDER}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    FFMPEG_PATH=${GIT_FOLDER}/macos_build_temp/FFmpeg-5.0.1/build_${ARCHITECTURE}/ffmpeg
    cp "${FFMPEG_PATH}" "${PACKAGE_FOLDER}/lib"
    if [ $? -ne 0 ]
    then
        echo "### Could not copy the file: ${FFMPEG_PATH}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    FFPROBE=${GIT_FOLDER}/macos_build_temp/FFmpeg-5.0.1/build_${ARCHITECTURE}/ffprobe
    cp "${FFPROBE}" "${PACKAGE_FOLDER}/lib"
    if [ $? -ne 0 ]
    then
        echo "### Could not copy the file: ${FFPROBE}"
        cd "${INITIAL_WORKING_FOLDER}"
        exit 1
    fi

    cp "${SCRIPT_PATH}" "${PACKAGE_FOLDER}"

    echo "### Compressing the package ..."
    cd ${PACKAGE_FOLDER_NAME}
    tar -cJf "../${PACKAGE_FOLDER_NAME}.tar.xz" .

    cd "${INITIAL_WORKING_FOLDER}"
}

# SCRIPT START

ARCHITECTURE=$(uname -m)

while [ $# -gt 0 ]; do
    case "$1" in
        --architecture=*)
            ARCHITECTURE="${1#*=}"
            ;;
        *)
            echo "### Error: Invalid command line parameter. Please use --architecture=..."
            exit 1
    esac
    shift
done

INITIAL_WORKING_FOLDER=${PWD}
SCRIPT_PATH=$(realpath $0)
GIT_FOLDER_NAME=obs-deps-${ARCHITECTURE}
GIT_FOLDER=${PWD}/${GIT_FOLDER_NAME}
GIT_TAG=2022-08-02

# Check if git is available
if ! command -v git &> /dev/null
then
    echo "'git' could not be found. Please install 'git' and start the script again."
    exit 1
fi

# Download the sources
download_obs_deps

# Patch to disable HEVC
patch_obs_deps

# Build
build_obs_deps

# Package
package_obs_deps