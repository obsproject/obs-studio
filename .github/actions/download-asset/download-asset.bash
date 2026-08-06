#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

shopt -s extglob

download-release-asset() {
  download_pattern="${release_patterns["${platform_tuple}"]:-}"

  if [[ -z "${download_pattern}" ]]; then
    echo "::error::No download pattern for tuple '${platform_tuple}'."
    return 1
  fi

  gh release download "${TAG_NAME}" \
    --pattern "${download_pattern}" \
    --dir "${DESTINATION}" \
    --clobber
}

download-tag-asset() {
  download_pattern="${release_patterns["${platform_tuple}"]:-}"

  if [[ -z "${download_pattern}" ]]; then
    echo "::error::No download pattern for tuple '${platform_tuple}'."
    return 1
  fi

  if [[ "${TAG_NAME}" =~ [0-9]+\.[0-9]+\.[0-9]+(-(rc|beta)[0-9]+)*$ ]]; then
    gh release download "${TAG_NAME}" \
      --pattern "${download_pattern}" \
      --dir "${DESTINATION}" \
      --clobber
  fi
}

download-artifact() {
  download_pattern="${artifact_patterns["${platform_tuple}"]:-}"

  if [[ -z "${download_pattern}" ]]; then
    echo "::error::No download pattern for tuple '${platform_tuple}'."
    return 1
  fi

  # FIXME: Remove early return once 'gh' supports artifacts generated with 'archive: false'.
  return

  gh run download "${GITHUB_RUN_ID}" \
    --pattern "${download_pattern}" \
    --dir "${DESTINATION}"
}

download-assets() {
  local -A release_patterns=(
    [macos-arm64]="OBS-Studio-*-macOS-Apple${PATTERN:-}.dmg"
    [macos-x86_64]="OBS-Studio-*-macOS-Intel${PATTERN:-}.dmg"
    [windows-x86_64]="OBS-Studio-*-Windows-x64${PATTERN:-}.zip"
    [windows-arm64]="OBS-Studio-*-Windows-ARM64${PATTERN:-}.zip"
  )

  local -A artifact_patterns=(
    [macos-arm64]="obs-studio-macos-arm64-*${PATTERN:--unsigned}.dmg"
    [macos-x86_64]="obs-studio-macos-x86_64-*${PATTERN:--unsigned}.dmg"
    [windows-x86_64]="obs-studio-windows-x86_64-*${PATTERN:--unsigned}.zip"
    [windows-arm64]="obs-studio-windows-arm64-*${PATTERN:--unsigned}.zip"
  )

  local platform_tuple="${PLATFORM,,*}-${ARCHITECTURE,,*}"

  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    DESTINATION="$(cygpath --unix "${DESTINATION}")"
  fi

  mkdir -p "${DESTINATION}"

  local download_pattern
  case "${GITHUB_EVENT_NAME}" in
    release) download-release-asset ;;
    workflow_dispatch)
      case "${GITHUB_REF_TYPE}" in
        tag) download-tag-asset ;;
        *) download-artifact ;;
      esac
      ;;
    schedule|push) download-artifact ;;
    *)
      echo "::error::Unsupported GitHub event name '${GITHUB_EVENT_NAME}'"
      return 1
      ;;
  esac

  GLOBSORT='-mtime'
  local output

  local -a found_files=()
  if output="$(compgen -G "${DESTINATION}/${download_pattern}")"; then
    local file
    while read -r file; do
      found_files+=("${file}")
    done <<< "${output}"
  fi

  if (( ! ${#found_files[@]} )); then
    echo "::warning::No downloaded files found with pattern '${download_pattern}'."
  else
    local file_path
    if [[ "${RUNNER_OS}" == 'Windows' ]]; then
      file_path="$(cygpath --windows "${found_files[0]}")"
    else
      file_path="${found_files[0]}"
    fi
    echo "path=${file_path}" >> "${GITHUB_OUTPUT}"
  fi
}

download-assets
