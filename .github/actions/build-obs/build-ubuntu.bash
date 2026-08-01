#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

build-ubuntu() {
  local checkout="${PWD}"
  if ! [[ -d "${checkout}/.git" && -r "${checkout}/CMakePresets.json" ]]; then
    echo '::error::Action needs to be run from the root directory of an obs-studio checkout.'
    return 1
  fi

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cmake --version
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

  declare -x CLICOLOR_FORCE=1

  echo '::group::Configure obs-studio'
  local -a cmake_args=(
    --preset ubuntu-ci
    -B "${build_dir}"
    -DENABLE_BROWSER:BOOL=ON
    -DCEF_ROOT_DIR:PATH="${build_dir}/.deps/cef_binary_${CEF_VERSION}_linux_${BUILD_TARGET}"
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cmake_args+=(--log-level=DEBUG)
  fi

  /usr/bin/cmake "${cmake_args[@]}"
  echo '::endgroup::'

  echo '::group::Build obs-studio'
  local -a cmake_build_args=(
    --build "${build_dir}"
    --config "${BUILD_CONFIG}"
    --parallel
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cmake_build_args+=(--verbose);
  fi

  /usr/bin/cmake "${cmake_build_args[@]}"
  echo '::endgroup::'

  echo '::group::CCache Statistics'
  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    ccache --show-stats --verbose --verbose
  else
    ccache --show-stats
  fi
  echo '::endgroup::'
}

build-ubuntu
