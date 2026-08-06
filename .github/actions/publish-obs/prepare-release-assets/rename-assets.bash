#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

shopt -s extglob

rename-assets() {
  local -A platform_suffixes=(
    [windows-x86_64-unsigned]=Windows-x64.zip
    [windows-arm64-unsigned]=Windows-ARM64.zip
    [macos-arm64-unsigned]=macOS-Apple.dmg
    [macos-x86_64-unsigned]=macOS-Intel.dmg
    [macos-arm64-signed]=macOS-Apple.dmg
    [macos-x86_64-signed]=macOS-Intel.dmg
    [windows-x86_64-signed]=Windows-x64.zip
    [windows-arm64-signed]=Windows-ARM64.zip
    [ubuntu-24.04-x86_64]=Ubuntu-24.04-x86_64.deb
    [ubuntu-26.04-x86_64]=Ubuntu-26.04-x86_64.deb
    [macos-arm64-debug-symbols]=macOS-Apple-dSYMs.tar.xz
    [macos-x86_64-debug-symbols]=macOS-Intel-dSYMs.tar.xz
    [ubuntu-24.04-x86_64-debug-symbols]=Ubuntu-24.04-x86_64-dbsym.ddeb
    [ubuntu-26.04-x86_64-debug-symbols]=Ubuntu-26.04-x86_64-dbsym.ddeb
    [windows-x86_64-debug-symbols]=Windows-x64-PDBs.zip
    [windows-arm64-debug-symbols]=Windows-ARM64-PDBs.zip
    [macos-arm64-plugin-dev]=macOS-Apple-Libraries.tar.xz
    [macos-x86_64-plugin-dev]=macOS-Intel-Libraries.tar.xz
    [windows-x86_64-plugin-dev]=Windows-x64-Libraries.zip
    [windows-arm64-plugin-dev]=Windows-ARM64-Libraries.zip
    [windows-x86_64-installer]=Windows-x64-Installer.exe
    [ubuntu-24.04-x86_64-sources]=Ubuntu-24.04-x86_64-Sources.tar.gz
  )

  local -A has_signed_asset=(
    [windows-x86_64]=0
    [windows-arm64]=0
    [macos-arm64]=0
    [macos-x86_64]=0
  )

  local platform_glob="(macos|windows|ubuntu-*)"
  local arch_glob="(arm64|x86_64)"
  local type_glob="(debug-symbols|sources|installer|unsigned|signed|plugin-dev)"
  local extension_glob="(dmg|tar.xz|zip|exe|deb|ddeb)"

  : "${ARTIFACT_PATTERN:="obs-studio-@${platform_glob}-@${arch_glob}-*@(-@${type_glob}).@${extension_glob}"}"

  local -a release_files=()
  local glob_output
  if glob_output="$(compgen -G "${PWD}/${ARTIFACT_PATTERN}/${ARTIFACT_PATTERN}")"; then
    glob_output="$(sort --version-sort <<< "${glob_output}")"

    local platform
    local output_name
    local file_name
    local regex
    while read -r file; do
      file_name="$(basename "${file}")"

      regex="obs-studio-(macos|windows|ubuntu-[^-]+)-${arch_glob}-[^-]+(-${type_glob})\.${extension_glob}$"

      if [[ "${file}" =~ ${regex} ]]; then
        platform="${BASH_REMATCH[1]}-${BASH_REMATCH[2]}"
        platform_suffix="${platform_suffixes["${platform}-${BASH_REMATCH[4]}"]:-}"

        if [[ -n "${platform_suffix}" ]]; then
          if [[ "${BASH_REMATCH[4]}" == 'signed' ]]; then
            has_signed_asset+=(["${platform}"]=1)
          fi

          output_name="OBS-Studio-${RELEASE_VERSION}-${platform_suffix}"

          if [[ "${BASH_REMATCH[4]}" == 'unsigned' ]] && (( ${has_signed_asset["${platform}"]} )); then
            continue
          fi

          release_files+=("${output_name}")

          mv "${file}" "${PWD}/${output_name}"
        fi
      else
        echo "::warning::File name '${file_name}' does not have a supported OBS Studio artifact name."
        continue
      fi
    done <<< "${glob_output}"
  else
    echo "::warning::No supported release artifacts found using '${PWD}/${ARTIFACT_PATTERN}'."
    return 1
  fi

  {
    if (( ${#release_files[@]} )); then
      local json_string
      json_string="$(jq --compact-output --monochrome-output --raw-input '
        .,inputs | split(" ")
      ' <<< "${release_files[@]}")"

      echo "has-release-files=true"
      echo "release-files=${json_string}"
    else
      echo "has-releaese-files=false"
      echo "release-files=[]"
    fi
  } >> "${GITHUB_OUTPUT}"
}

rename-assets
