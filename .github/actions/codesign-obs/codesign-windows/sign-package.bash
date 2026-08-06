#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

shopt -s globstar
shopt -s extglob

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

sign-project() {
  local signtool_location="${PROGRAMFILES} (x86)\\Windows Kits\\10\\App Certification Kit\\signtool.exe"
  local signtool
  signtool=$(cygpath --unix "${PROGRAMFILES} (x86)/Windows Kits/10/App Certification Kit/signtool.exe")

  if [[ ! -r "${signtool}" ]]; then
    echo "::error::Signtool not found at '${signtool_location}'."
    return 1
  fi

  ARTIFACT_PATH="$(cygpath --unix "${ARTIFACT_PATH}")"

  local key_project="projects/ci-signing"
  local key_location="locations/global"
  local key_ring="keyRings/production"
  local key_name="cryptoKeys/release-sign-hsm"
  local key_version="cryptoKeyVersions/1"
  local kms_key="${key_project}/${key_location}/${key_ring}/${key_name}/${key_version}"

  local -a signtool_arguments=(
    "//fd" 'sha384'
    "//as"
    "//tr" "http://timestamp.digicert.com"
    "//td" 'sha256'
    "//f" "${GITHUB_ACTION_PATH}/prod.crt"
    "//csp" 'Google Cloud KMS Provider'
    "//kc" "${kms_key}"
  )

  local -a project_files=()
  local output
  if output="$(compgen -G "${ARTIFACT_PATH}/**/*.@(exe|dll|pyd)")"; then
    while read -r file; do
      project_files+=("${file}")
    done <<< "${output}"
  fi

  if (( ! ${#project_files[@]} )); then
    echo "::warning::No OBS Studio files found in '${ARTIFACT_PATH}."
    return 0
  fi


  local -i chunk_size=5
  local -i num_items="${#project_files[@]}"
  local -i num_chunks="$(( (num_items + chunk_size - 1) / chunk_size))"
  local -i seq_end="$(( num_chunks - 1 ))"

  local sequence
  sequence="$(seq 0 "${seq_end}")"

  local i
  while read -r i; do
    local -i start_index=$(( i * chunk_size ))
    local -a slice=("${project_files[@]:${start_index}:${chunk_size}}")

    "${signtool}" sign "${signtool_arguments[@]}" "${slice[@]}"
  done <<< "${sequence}"
}

sign-project
