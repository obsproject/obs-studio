#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

shopt -s extglob

run_steamcmd() {
  if [[ ! -d "${STEAMCMD_PATH}" ]]; then
    echo "::error::steamcmd not found in '${STEAMCMD_PATH}'."
    return 1
  fi

  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    STEAMCMD_PATH="$(cygpath --unix "${STEAMCMD_PATH}")"
  fi

  local steamcmd_invocation
  case "${RUNNER_OS,,*}" in
    linux|macos)
      steamcmd_invocation="bash ${STEAMCMD_PATH}/steamcmd.sh"
      ;;
    windows)
      steamcmd_invocation="${STEAMCMD_PATH}/steamcmd.exe"
      ;;
    *)
      echo "Unsupported runner operating system '${RUNNER_OS}'."
      return 1
      ;;
  esac

  local preview=''
  if [[ "${DRY_RUN:-false}" == 'true' ]]; then
    preview='true'
  fi

  echo '::group::Running steamcmd...'
  ${steamcmd_invocation} \
    +login "${STEAM_USER}" "${STEAM_PASSWORD}" "${STEAM_TOTP_CODE}" \
    +run_app_build "${preview:+-preview}" "${build_file_output}" \
    +quit
  echo '::endgroup::'


  local log_path="${PWD}/build/*"
  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    log_path="$(cygpath --windows "${log_path}")"
  fi

  echo "log-path=${log_path}" >> "${GITHUB_OUTPUT}"
}

upload-assets() {
  if [[ ! -d "${ASSET_PATH}" ]]; then
    echo "::error::Provided asset path '${ASSET_PATH}' not found."
    return 1
  fi

  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    ASSET_PATH="$(cygpath --unix "${ASSET_PATH}")"
  fi

  local build_file
  if [[ "${USE_PLAYTEST:-false}" == 'true' ]]; then
    build_file="${GITHUB_ACTION_PATH}/build/obs_playtest_build.vdf"
  else
    build_file="${GITHUB_ACTION_PATH}/build/obs_build.vdf"
  fi

  if [[ ! -r "${build_file}" ]]; then
    echo "::error::Steam manifest file not found in GitHub workspace."
    return 1
  fi

  pushd "${ASSET_PATH}" > /dev/null

  local macos_assets
  if ! macos_assets="$(compgen -G "steam-asset-macos-*.tar.xz")"; then
    echo "::error::No macOS release assets found."
    return 1
  fi

  local macos_asset
  while read -r macos_asset; do
    echo "::group::Extracting macOS asset '${macos_asset}'"
    tar --extract --verbose --xz --file "${macos_asset}"
    rm "${macos_asset}"
    echo '::endgroup::'
  done <<< "${macos_assets}"

  local desc_replacement="${BRANCH_NAME}-${DESCRIPTION}"
  local branch_replacement="${BRANCH_NAME}"

  local build_file_output="${ASSET_PATH}/build.vdf"
  sed "s/@@DESC@@/${desc_replacement}/;s/@@BRANCH@@/${branch_replacement}/" \
    "${build_file}" > "${build_file_output}"

  local build_file_contents
  build_file_contents="$(<"${build_file_output}")"

  echo -e "Generated ${build_file_output}:\n${build_file_contents}"

  run_steamcmd

  popd > /dev/null
}

upload-assets
