#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

shopt -s nocaseglob

create-archive() {
  echo '::group::Create Windows Archive'
  pushd "${build_dir}" > /dev/null

  local -a cpack_args=(
    -C "${BUILD_CONFIG}"
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cpack_args+=(--verbose)
  fi

  cpack "${cpack_args[@]}"

  local output
  if output="$(compgen -G "obs-studio-*-windows-${BUILD_TARGET}.zip")"; then
    mv "${output}" "${OUTPUT_PATH}/${artifact_name}-unsigned.zip"
  fi

  popd > /dev/null
  echo '::endgroup::'
}

create-developer-archive() {
  echo '::group::Create Libraries For Plugin Development'

  local install_dir="${OUTPUT_PATH}/libobs_release"

  cmake --install "${build_dir}" --component Development --config Release --prefix "${install_dir}"

  pushd "${install_dir}" > /dev/null
  local artifact_name="${artifact_name}-plugin-dev.zip"

  7z a "${OUTPUT_PATH}/${artifact_name}" "${PWD}"/*

  popd > /dev/null
  echo '::endgroup::'
}

package-windows() {
  local checkout="${PWD}"
  if ! [[ -d "${checkout}/.git" && -r "${checkout}/CMakePresets.json" ]]; then
    echo '::error::Action needs to be run from an obs-studio checkout root directory'
    return 1
  fi

  OUTPUT_PATH="$(cygpath --unix "${OUTPUT_PATH}")"

  local build_dir
  {
    local preset_build_dir
    preset_build_dir="$(jq --raw-output --arg platform "windows-${BUILD_TARGET}" '
      .configurePresets[] | select(.name == $platform) | .binaryDir
    ' "${checkout}/CMakePresets.json")"
    build_dir="${preset_build_dir//\$\{sourceDir\}/"${OUTPUT_PATH}"}"
  }

  local -A commit_info
  {
    local git_description
    git_description="$(git describe --tags --long)"

    local version_regex='^([0-9]+\.[0-9]+\.[0-9]+(-(rc|beta).+)?)-([0-9]+)-g([[:alnum:]]+)$'

    if [[ "${git_description}" =~ ${version_regex} ]]; then
      commit_info=(
        [version]="${BASH_REMATCH[1]}"
        [distance]="${BASH_REMATCH[-2]}"
        [hash]="${BASH_REMATCH[-1]}"
      )
    else
      echo '::error::Unable to detect version from git commit.'
      return 1
    fi
  }

  local artifact_name="${OUTPUT_NAME:-"obs-studio-windows-${BUILD_TARGET}-${commit_info[hash]}"}"

  create-archive

  if [[ "${BUILD_CONFIG}" == 'Release' ]]; then
    create-developer-archive
  fi
}

package-windows
