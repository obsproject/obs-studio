#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

convert-to-sarif() {
  local -a pvs_conversion_arguments=(
    --analyzer 'GA:1,2'
    --excludedCodes 'V1042,Renew'
    --renderTypes 'Sarif'
    --outputDir "${RUNNER_TEMP}"
  )

  local pvs_converter_location
  pvs_converter_location="$(cygpath --unix "${PROGRAMFILES} (x86)/PVS-Studio/PlogConverter.exe")"

  "${pvs_converter_location}" "${pvs_conversion_arguments[@]}" "${RUNNER_TEMP}/pvs-analysis.plog"

  local expected_sarif_file="${RUNNER_TEMP}/pvs-analysis.plog.sarif"
  if [[ ! -r "${expected_sarif_file}" ]]; then
    echo "::error::Generated SARIF file '${expected_sarif_file}' not found."
    return 1
  fi
}

run-pvs-studio() {
  BUILD_SOLUTION="$(cygpath --unix "${BUILD_SOLUTION}")"

  local output
  if ! output="$(compgen -G "${BUILD_SOLUTION}")"; then
    echo "::error::Unable to find Visual Studio build solution via '${BUILD_SOLUTION}'."
    return 1
  fi

  local -a pvs_arguments=(
    --progress
    --disableLicenseExpirationCheck
    --platform "${BUILD_ARCHITECTURE}"
    --configuration "${BUILD_CONFIG}"
    --target "${output}"
    --output "${RUNNER_TEMP}/pvs-analysis.plog"
    --rulesConfig "${GITHUB_ACTION_PATH}/obs.pvsconfig"
  )

  local pvs_location
  pvs_location="$(cygpath --unix "${PROGRAMFILES} (x86)/PVS-Studio/PVS-Studio_Cmd.exe")"

  # Acceptable error codes per https://pvs-studio.com/en/docs/manual/0035/
  if ! "${pvs_location}" "${pvs_arguments[@]}"; then
    local -i accepted_code_mask="$(( 1024 | 256 ))"

    local -i return_code="${?}"

    if ! (( return_code & accepted_code_mask )); then
      echo "::error::PVS-Studio exited with unsupported error code ${return_code}."
      return 1
    fi
  fi

  convert-to-sarif
}

run-pvs-studio
