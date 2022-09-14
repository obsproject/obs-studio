export PATH=/usr/local/opt/ccache/libexec:$PATH
set -e
set -v

mkdir packed_build
PACKED_BUILD=$PWD/packed_build
mkdir build

GENERATOR="Xcode"

CHECKOUT_DIR="$(/usr/bin/git rev-parse --show-toplevel)"
source "${CHECKOUT_DIR}/slobs_CI/01_install_dependencies.sh"
DEPS_BUILD_DIR="${CHECKOUT_DIR}/../obs-build-dependencies"
BUILD_DIR="${CHECKOUT_DIR}/build"

cmake \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-${CI_MACOSX_DEPLOYMENT_TARGET}} \
    -S ${CHECKOUT_DIR} -B ${BUILD_DIR} \
    -G ${GENERATOR} \
    -DVLC_PATH="${DEPS_BUILD_DIR}/vlc-${VLC_VERSION:-${CI_VLC_VERSION}}" \
    -DENABLE_VLC=ON \
    -DCEF_ROOT_DIR="${DEPS_BUILD_DIR}/cef_binary_${MACOS_CEF_BUILD_VERSION:-${CI_MACOS_CEF_VERSION}}_macos_${ARCH:-x86_64}" \
    -DBROWSER_LEGACY=$(test "${MACOS_CEF_BUILD_VERSION:-${CI_MACOS_CEF_VERSION}}" -le 3770 && echo "ON" || echo "OFF") \
    -DCMAKE_PREFIX_PATH="${DEPS_BUILD_DIR}/obs-deps" \
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_ARCHS} \
    -DCMAKE_INSTALL_PREFIX=$PACKED_BUILD \
    -DCMAKE_BUILD_TYPE=${BUILD_CONFIG:-${CI_BUILD_CONFIG}} \
    -DCOPIED_DEPENDENCIES=false \
    -DCOPY_DEPENDENCIES=true \
    -DENABLE_SCRIPTING=false \
    -DBROWSER_FRONTEND_API_SUPPORT=false \
    -DBROWSER_PANEL_SUPPORT=false \
    -DUSE_UI_LOOP=true \
    -DCHECK_FOR_SERVICE_UPDATES=true \
    -DOBS_CODESIGN_LINKER=false \
    -DWEBRTC_INCLUDE_PATH=$DEPS_DIR/webrtc_dist \
    -DWEBRTC_LIB_PATH=$DEPS_DIR/webrtc_dist/libwebrtc.a \
    -DMEDIASOUP_INCLUDE_PATH=$DEPS_DIR/libmediasoupclient_dist/include/mediasoupclient/ \
    -DMEDIASOUP_LIB_PATH=$DEPS_DIR/libmediasoupclient_dist/lib/libmediasoupclient.a \
    -DMEDIASOUP_SDP_LIB_PATH=$DEPS_DIR/libmediasoupclient_dist/lib/libsdptransform.a \
    -DMEDIASOUP_SDP_INCLUDE_PATH=$DEPS_DIR/libmediasoupclient_dist/include/sdptransform \
    -DOPENSSL_CRYPTO_LIBRARY=/usr/local/opt/openssl@3/lib/libcrypto.a \
    -DOPENSSL_INCLUDE_DIR=/usr/local/opt/openssl@3/include \
    -DOPENSSL_SSL_LIBRARY=/usr/local/opt/openssl@3/lib/libssl.a \
    ${QUIET:+-Wno-deprecated -Wno-dev --log-level=ERROR}

cmake --build ${BUILD_DIR} --target install --config ${BUILD_CONFIG:-${CI_BUILD_CONFIG}} -v