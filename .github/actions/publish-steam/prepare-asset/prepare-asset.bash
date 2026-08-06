#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

prepare-windows-asset() {
  local script_source
  script_source="$(cygpath --unix "${GITHUB_ACTION_PATH}/scripts/windows")"
  local script_destination
  script_destination="$(cygpath --unix "${RUNNER_TEMP}/steam-asset/steam-windows")"

  mkdir -p "${script_destination}/scripts"
  pushd "${script_destination}" > /dev/null

  unzip -q "${ASSET}"
  rm "${ASSET}"

  cp -r "${script_source}" "${script_destination}/scripts"

  touch "${script_destination}/disable_updater"

  popd > /dev/null

  local asset_path
  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    asset_path="$(cygpath --windows "${RUNNER_TEMP}/steam-asset")"
  else
    asset_path="${RUNNER_TEMP}/steam-asset"
  fi

  echo "asset-path=${asset_path}" >> "${GITHUB_OUTPUT}"
}

prepare-macos-asset() {
  if [[ "${RUNNER_OS,,*}" != 'macos' ]]; then
    echo '::error::Preparing macOS Steam assets requires a macOS runner'
    return 1
  fi

  mkdir -p "${RUNNER_TEMP}/steam-asset/steam-macos/${ARCHITECTURE}/OBS.app"
  hdiutil attach \
    -noverify \
    -readonly \
    -noautoopen \
    -mountpoint /Volumes/obs-studio \
    "${ASSET}"
  ditto /Volumes/obs-studio/OBS.app "${RUNNER_TEMP}/steam-asset/steam-macos/${ARCHITECTURE}/OBS.app"
  hdiutil unmount /Volumes/obs-studio

  cp -r "${GITHUB_ACTION_PATH}/scripts/macos/launch.sh" "${RUNNER_TEMP}/steam-asset/steam-macos/launch.sh"

  pushd "${RUNNER_TEMP}/steam-asset" > /dev/null
  XZ_OPT='-1 --threads=0' tar --create --verbose --xz --file \
    "${RUNNER_TEMP}/steam-asset-macos-${ARCHITECTURE}.tar.xz" steam-macos
  popd > /dev/null

  echo "asset-path=${RUNNER_TEMP}/steam-asset-macos-${ARCHITECTURE}.tar.xz" >> "${GITHUB_OUTPUT}"
}

prepare-asset() {
  if [[ -z "${ASSET}" ]]; then
    echo "::error::No asset provided.."
    return 1
  fi

  local -A release_patterns=(
    [macos-arm64]="OBS-Studio-*-macOS-Apple.dmg"
    [macos-x86_64]="OBS-Studio-*-macOS-Intel.dmg"
    [windows-x86_64]="OBS-Studio-*-Windows-x64.zip"
    [windows-arm64]="OBS-Studio-*-Windows-ARM64.zip"
  )

  local -A artifact_patterns=(
    [macos-arm64]="obs-studio-macos-arm64-*-unsigned.dmg"
    [macos-x86_64]="obs-studio-macos-x86_64-*-unsigned.dmg"
    [windows-x86_64]="obs-studio-windows-x86_64-*-unsigned.zip"
    [windows-arm64]="obs-studio-windows-arm64-*-unsigned.zip"
  )

  local platform_tuple="${PLATFORM,,*}-${ARCHITECTURE,,*}"
  local expected_file_name

  if [[ "${GITHUB_REF_TYPE}" == 'tag' ]]; then
    expected_file_name="${release_patterns["${platform_tuple}"]:-}"
  else
    expected_file_name="${artifact_patterns["${platform_tuple}"]:-}"
  fi

  if [[ -z "${expected_file_name}" ]]; then
    echo "::error::Unsupported platform tuple '${platform_tuple}'."
    return 1
  fi

  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    ASSET="$(cygpath --unix "${ASSET}")"
  fi

  if [[ ! -r "${ASSET}" && "${ASSET}" != */${expected_file_name} ]]; then
    echo "::error::Expected asset '${expected_file_name}' not found."
    return 1
  fi

  case "${PLATFORM,,*}" in
    windows) prepare-windows-asset ;;
    macos) prepare-macos-asset ;;
    *)
      echo "::error::Unsupported platform '${PLATFORM}'."
      return 1
      ;;
  esac
}

prepare-asset
