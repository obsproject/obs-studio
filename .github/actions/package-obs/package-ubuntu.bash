#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

shopt -s extglob

create-deb-package() {
  echo '::group::Create deb package'
  local -a cmake_args=(
    --build "${build_dir}"
    --config "${BUILD_CONFIG}"
    --target package
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cmake_args+=(--verbose)
  fi

  /usr/bin/cmake "${cmake_args[@]}"

  local actual_name
  pushd "${build_dir}" > /dev/null
  files=(obs-studio-*-Linux*.@(ddeb|deb))
  for file in "${files[@]}"; do
    actual_name="${file//obs-studio-*-Linux/"${artifact_name}"}"
    actual_name="${actual_name//-dbgsym.ddeb/-debug-symbols.ddeb}"
    mv "${file}" "${OUTPUT_PATH}/${actual_name}"
  done
  popd > /dev/null
  echo '::endgroup::'
}

create-archive() {
  echo '::group::Install obs-studio'
  local install_dir="${OUTPUT_PATH}/install/${BUILD_CONFIG}"

  local -a cmake_install_args=(
    --install "${build_dir}"
    --prefix "${install_dir}"
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    cmake_install_args+=(--verbose);
  fi

  /usr/bin/cmake "${cmake_install_args[@]}"
  echo '::endgroup::'

  echo '::group::Create archive'
  pushd "${install_dir}" > /dev/null
  tar --create --verbose --file \
    "${OUTPUT_PATH}/${artifact_name}.tar.gz" @(bin|lib|share)
  popd > /dev/null
  echo '::endgroup::'
}

create-tarball() {
  echo '::group::Create sources tarball'

  local -a cmake_args=(
    --build "${build_dir}"
    --config "${BUILD_CONFIG}"
    --target package_source
  )

  /usr/bin/cmake "${cmake_args[@]}"

  local artifact_name="${artifact_name}-sources"

  pushd "${build_dir}" > /dev/null
  files=(obs-studio-*-sources.tar.*)
  for file in "${files[@]}"; do
    mv "${file}" "${OUTPUT_PATH}/${file//obs-studio-*-sources/"${artifact_name}"}"
  done

  popd > /dev/null
  echo '::endgroup::'
}

create-developer-archive() {
  echo '::group::Create Libraries For Plugin Development'
  local install_dir="${OUTPUT_PATH}/libobs_release/${BUILD_CONFIG}"
  cmake --install "${build_dir}" --component Development --config Release --prefix "${install_dir}"

  local artifact_name="${artifact_name}-plugin-dev"

  pushd "${install_dir}" > /dev/null
  tar --create --verbose --file \
    "${build_dir}/${artifact_name}".tar.gz @(include|lib|share)
  popd > /dev/null
  echo '::endgroup::'
}

package-ubuntu() {
  local checkout="${PWD}"
  if ! [[ -d "${checkout}/.git" && -r "${checkout}/CMakePresets.json" ]]; then
    echo '::error::Action needs to be run from an obs-studio checkout root directory'
    return 1
  fi

  local build_dir
  {
    local preset_build_dir
    preset_build_dir="$(jq --raw-output '
      .configurePresets[] | select(.name == "ubuntu") | .binaryDir
    ' "${checkout}/CMakePresets.json")"
    build_dir="${preset_build_dir//\$\{sourceDir\}/"${OUTPUT_PATH}"}"
  }

  declare -x CLICOLOR_FORCE=1

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

  local artifact_name="${OUTPUT_NAME:-"obs-studio-ubuntu-${BUILD_TARGET}-${commit_info[hash]}"}"
  echo '::endgroup::'

  if [[ "${BUILD_PACKAGE:-false}" == 'true' ]]; then
    create-deb-package
  else
    create-archive
  fi

  if [[ "${BUILD_CONFIG}" == 'Release' ]]; then
    create-tarball

    create-developer-archive
  fi
}

package-ubuntu
