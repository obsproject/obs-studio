#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

build-windows() {
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
    preset_build_dir="$(jq --raw-output --arg platform "windows-${BUILD_TARGET}" '
      .configurePresets[] | select(.name == $platform) | .binaryDir
    ' "${checkout}/CMakePresets.json")"
    build_dir="${preset_build_dir//\$\{sourceDir\}/"${OUTPUT_PATH}"}"
  }

  echo '::group::Configure obs-studio'
  local -a cmake_args=(
    --preset "windows-ci-${BUILD_TARGET}"
    -B "${build_dir}"
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cmake_args+=(--log-level=DEBUG)
  fi

  cmake "${cmake_args[@]}"
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

  cmake "${cmake_build_args[@]}" -- //consoleLoggerParameters:Summary //nologo
  echo '::endgroup::'
}

build-windows
