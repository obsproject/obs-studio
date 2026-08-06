#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

sign-gamecapture() {
  local signtool_location="${PROGRAMFILES} (x86)\\Windows Kits\\10\\App Certification Kit\\signtool.exe"
  local signtool
  signtool=$(cygpath --unix "${PROGRAMFILES} (x86)/Windows Kits/10/App Certification Kit/signtool.exe")

  if [[ ! -r "${signtool}" ]]; then
    echo "::error::Signtool not found at '${signtool_location}'."
    return 1
  fi

  ARTIFACT_PATH="$(cygpath --unix "${ARTIFACT_PATH}")"

  local gamecapture_path="${ARTIFACT_PATH}/data/obs-plugins/win-capture"

  if [[ ! -r "${gamecapture_path}" ]]; then
    local windows_path
    windows_path="$(cygpath --windows "${gamecapture_path}")"
    echo "::error::No game capture module found at '${windows_path}'."
    return 1
  fi

  local key_project="projects/ci-signing"
  local key_location="locations/global"
  local key_ring="keyRings/production"
  local key_name="cryptoKeys/game-capture-release-sign-hsm"
  local key_version="cryptoKeyVersions/1"
  local kms_key="${key_project}/${key_location}/${key_ring}/${key_name}/${key_version}"

  local -a signtool_arguments=(
    "//fd" sha256
    "//t" "http://timestamp.digicert.com"
    "//f" "${GITHUB_ACTION_PATH}/prod-gc.crt"
    "//csp" "Google Cloud KMS Provider"
    "//kc" "${kms_key}"
  )

  local output
  local -a gamecapture_dlls=()
  if output="$(compgen -G "${gamecapture_path}/*.dll")"; then
    while read -r file; do
      gamecapture_dlls+=("${file}")
    done <<< "${output}"
  fi

  if (( ! ${#gamecapture_dlls[@]} )); then
    echo "::warning::No game capture files found in '${gamecapture_path}."
    return 0
  fi

  "${signtool}" sign "${signtool_arguments[@]}" "${gamecapture_dlls[@]}"
}

sign-gamecapture
