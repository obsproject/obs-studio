#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

download-custom-assets() {
  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    DESTINATION="$(cygpath --unix "${DESTINATION}")"
  fi

  case "${GITHUB_EVENT_NAME}" in
    workflow_dispatch)
      mkdir -p "${DESTINATION}"

      local -A url_patterns=(
        [macos-arm64]='OBS-Studio-.+-macOS-Apple.dmg'
        [macos-x86_64]='OBS-Studio-.+-macOS-Intel.dmg'
        [windows-x86_64]='OBS-Studio-.+-Windows-x64.zip'
        [windows-arm64]='OBS-Studio-.+-Windows-ARM64.zip'
      )

      local platform_tuple="${PLATFORM,,*}-${ARCHITECTURE,,*}"

      local url_regex="^https:\/\/.+\/.+\/${url_patterns["${platform_tuple}"]}$"

      if [[ "${CUSTOM_ASSET}" =~ ${url_regex} ]]; then
        local file_name
        file_name="$(basename "${CUSTOM_ASSET}")"
        local file_root="${file_name%%.*}"
        local file_extension="${file_name#*.}"
        local custom_file_name="${file_root}-custom.${file_extension}"

        curl \
          --location \
          --output "${DESTINATION}/${custom_file_name}" \
          -- "${CUSTOM_ASSET}"

        if [[ ! -r "${DESTINATION}/${file_name}" ]]; then
          echo "::warning::Custom asset ${file_name} does not replace an existing release asset."
        else
          rm -r "${DESTINATION:?}/${file_name}"
        fi

        mv "${DESTINATION}/${custom_file_name}" "${DESTINATION}/${file_name}"
      else
        echo "::error::Unsupported custom asset url '${CUSTOM_ASSET}' provided."
        return 1
      fi

      ;;
    *)
      echo "::error::Custom asset download only available for 'workflow_dispatch' events."
      return 1
      ;;
  esac
}

download-custom-assets
