#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-pvs-studio() {
  if [[ -z "${PVS_STUDIO_USERNAME}" || -z "${PVS_STUDIO_LICENSE}" ]]; then
    echo "::error::PVS-Studio setup requires a license username and license key."
    return 1
  fi

  local pvs_studio_regex='^https:\/\/files\.pvs-studio\.com/PVS-Studio_setup.exe$'
  if ! [[ "${PVS_STUDIO_URL}" =~ ${pvs_studio_regex} ]]; then
    echo "::error::Invalid PVS-Studio download url '${PVS_STUDIO_URL}'."
    return 1
  fi

  echo "::group::Download PVS-Studio ${PVS_STUDIO_VERSION}"
  curl \
    --location \
    --remote-name \
    --output-dir "${RUNNER_TEMP}" \
    -- "${PVS_STUDIO_URL}"

  local pvs_file_basename
  pvs_file_basename="$(basename "${PVS_STUDIO_URL}")"

  local shasum_result
  shasum_result="$(openssl dgst -sha256 "${RUNNER_TEMP}/${pvs_file_basename}")"
  local checksum
  read -r _ checksum <<< "${shasum_result}"

  if [[ "${PVS_STUDIO_CHECKSUM,,*}" != "${checksum}" ]]; then
    echo "::error::${pvs_file_basename} checksum mismatch: ${checksum} (expected: ${PVS_STUDIO_CHECKSUM,,*})."
    return 1
  fi
  echo '::endgroup::'

  echo '::group::Install PVS-Studio'
  local -a pvs_install_arguments=(
    "//components=Core"
    "//silent"
    "//supressmsgboxes"
    "//norestart"
    "//nocloseapplications"
    "//skipNetFrameworkInstallation"
  )

  "${RUNNER_TEMP}/${pvs_file_basename}" "${pvs_install_arguments[@]}"
  echo '::endgroup::'

  echo '::group::Activate PVS-Studio'
  local -a pvs_activate_arguments=(
    credentials --userName "${PVS_STUDIO_USERNAME}" --licenseKey "${PVS_STUDIO_LICENSE}"
  )
  local pvs_location
  pvs_location="$(cygpath --unix "${PROGRAMFILES} (x86)/PVS-Studio/PVS-Studio_Cmd.exe")"
  "${pvs_location}" "${pvs_activate_arguments[@]}"
  echo '::endgroup::'

}

setup-pvs-studio
