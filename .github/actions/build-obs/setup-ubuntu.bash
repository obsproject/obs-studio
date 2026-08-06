#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-system-packages() {
  echo '::group::Install System Packages'
  sudo apt-get update
  sudo apt-get install --yes --no-install-recommends \
    cmake ccache build-essential libglib2.0-dev \
    extra-cmake-modules lsb-release dh-cmake \
    libcurl4-openssl-dev \
    libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev \
    libswresample-dev libswscale-dev \
    libjansson-dev \
    libx11-xcb-dev \
    libgles2-mesa-dev \
    libwayland-dev \
    libpipewire-0.3-dev \
    libpulse-dev \
    libx264-dev \
    libmbedtls-dev \
    libgl1-mesa-dev \
    libjansson-dev \
    uthash-dev \
    libsimde-dev \
    libluajit-5.1-dev python3-dev \
    libx11-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-xinerama0-dev \
    libxcb-composite0-dev libxinerama-dev libxcb1-dev libx11-xcb-dev libxcb-xfixes0-dev \
    swig libcmocka-dev libxss-dev libglvnd-dev \
    libxkbcommon-dev libatk1.0-dev libatk-bridge2.0-dev libxcomposite-dev libxdamage-dev \
    libasound2-dev libfdk-aac-dev libfontconfig-dev libfreetype6-dev libjack-jackd2-dev \
    libpulse-dev libsndio-dev libspeexdsp-dev libudev-dev libv4l-dev libva-dev libvlc-dev \
    libpci-dev libdrm-dev \
    nlohmann-json3-dev libwebsocketpp-dev libasio-dev libqrcodegencpp-dev \
    libffmpeg-nvenc-dev librist-dev libsrt-openssl-dev \
    qt6-base-dev libqt6svg6-dev qt6-base-private-dev \
    libvpl-dev libvpl2
  echo '::endgroup::'
}

setup-prebuilt-packages() {
  local checkout="${PWD}"
  if ! [[ -d "${checkout}/.git" && -r "${checkout}/CMakePresets.json" ]]; then
    echo "::error::Action needs to be run from the root directory of an obs-studio checkout."
    return 1
  fi

  mkdir -p "${OUTPUT_PATH}"

  local build_dir
  {
    local preset_build_dir
    preset_build_dir="$(jq --raw-output '
      .configurePresets[] | select(.name == "ubuntu") | .binaryDir
    ' "${checkout}/CMakePresets.json")"
    build_dir="${preset_build_dir//\$\{sourceDir\}/"${OUTPUT_PATH}"}"
  }

  echo '::group::Fetch Prebuilt Dependencies'
  mkdir -p "${build_dir}/.deps"

  local deps_version
  local deps_baseurl
  local deps_hash

  local jq_result
  jq_result="$(jq --raw-output --arg target "ubuntu-${BUILD_TARGET}" '
    .configurePresets[]
    | select(.name == "dependencies")
    | .vendor["obsproject.com/obs-studio"].dependencies["cef"]
    | {version, baseUrl, "hash": .hashes[$target], "revision": .revision[$target]}
    | join(" ")
  ' "${checkout}/CMakePresets.json")"

  read -r deps_version deps_baseurl deps_hash deps_revision <<< "${jq_result}"

  if [[ -z "${deps_version}" ]]; then
    echo '::error::No valid CEF information found in CMakePresets file.'
    return 1
  fi

  pushd "${build_dir}/.deps" > /dev/null

  local filename="cef_binary_${deps_version}_linux_${BUILD_TARGET}${deps_revision:+"_v${deps_revision}"}.tar.xz"
  local url="${deps_baseurl}/${filename}"
  local target="cef_binary_${deps_version}_linux_${BUILD_TARGET}"
  echo "CEF_VERSION=${deps_version}" >> "${GITHUB_ENV}"

  curl \
    --show-error \
    --location \
    --remote-name \
    -- "${url}"

  local shasum_output
  shasum_output="$(openssl dgst -sha256 "${filename}")"
  read -r _ artifact_checksum <<< "${shasum_output}"

  if [[ "${deps_hash}" != "${artifact_checksum}" ]]; then
    echo '::error::Incorrect checksum of downloaded CEF dependency.'
    return 1;
  fi

  mkdir -p "${target}"
  pushd "${target}" > /dev/null
  XZ_OPT='--threads=0' tar --strip-components 1 --extract --xz --file "${build_dir}/.deps/${filename}"
  popd > /dev/null

  popd > /dev/null
  echo '::endgroup::'
}

setup-ubuntu() {
  setup-system-packages
  setup-prebuilt-packages

  if { command -v ccache >/dev/null; } 2>&1 ; then
    echo '::group::Setting up CCache'
    ccache --set-config=direct_mode=true
    ccache --set-config=inode_cache=true
    ccache --set-config=compiler_check=content
    ccache --set-config=file_clone=true
    ccache --set-config=sloppiness=include_file_mtine,include_file_ctime,file_stat_matches,system_headers
    ccache --set-config=cache_dir="${RUNNER_TEMP}/.ccache"
    ccache --set-config=max_size="${CCACHE_SIZE:-1G}"
    ccache --zero-stats > /dev/null

    local runner_os_version
    runner_os_version="$(lsb_release --release --short)"
    if (( "${runner_os_version%%.*}" == 24 )); then
      ccache --set-config=run_second_cpp=true
    fi

    ccache --show-config

    echo '::endgroup::'
  fi
}

setup-ubuntu
