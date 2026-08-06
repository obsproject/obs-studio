#!/usr/bin/env bash
# shellcheck disable=SC2154,SC1091

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

fetch-bouf() {

  if ! [[ "${bouf_file_url}" =~ ${bouf_file_regex} ]]; then
    echo "::error::Invalid BOUF download URL '${bouf_file_url}'."
    return 1
  fi

  echo "::group::Download BOUF ${BOUF_VERSION}"
  curl \
    --location \
    --remote-name \
    --output-dir "${RUNNER_TEMP}" \
    -- "${bouf_file_url}"

  local shasum_result
  shasum_result="$(openssl dgst -sha256 "${RUNNER_TEMP}/${bouf_file_basename}")"
  local checksum
  read -r _ checksum <<< "${shasum_result}"

  if [[ "${BOUF_CHECKSUM}" != "${checksum}" ]]; then
    echo "::error::${bouf_file_basename} checksum mismatch: ${checksum} (expected: ${BOUF_CHECKSUM})."
    return 1
  fi
  echo '::endgroup::'
}

fetch-nsis() {
  if ! [[ "${bouf_nsis_file_url}" =~ ${bouf_nsis_file_regex} ]]; then
    echo "::error::Invalid BOUF nsis files download URL '${bouf_nsis_file_url}'."
    return 1
  fi

  echo "::group::Download BOUF NSIS components ${BOUF_VERSION}"
  curl \
    --location \
    --remote-name \
    --output-dir "${RUNNER_TEMP}" \
    -- "${bouf_nsis_file_url}"

  local shasum_result
  shasum_result="$(openssl dgst -sha256 "${RUNNER_TEMP}/${bouf_nsis_file_basename}")"
  local checksum
  read -r _ checksum <<< "${shasum_result}"

  if [[ "${BOUF_NSIS_CHECKSUM}" != "${checksum}" ]]; then
    echo "::error::${bouf_nsis_file_basename} checksum mismatch: ${checksum} (expected: ${BOUF_NSIS_CHECKSUM})."
    return 1
  fi
  echo '::endgroup::'
}

setup-bouf() {
  local bouf_file_regex='^https:\/\/.+\/v[0-9\.]+\/bouf-windows-v[0-9\.]+\.zip$'
  local bouf_file_url="${BOUF_URL}/v${BOUF_VERSION}/bouf-windows-v${BOUF_VERSION}.zip"
  local bouf_file_basename
  bouf_file_basename="$(basename "${bouf_file_url}")"
  fetch-bouf

  local bouf_nsis_file_regex='^https:\/\/.+\/v[0-9\.]+\/bouf-nsis-v[0-9\.]+\.zip$'
  local bouf_nsis_file_url="${BOUF_URL}/v${BOUF_VERSION}/bouf-nsis-v${BOUF_VERSION}.zip"
  local bouf_nsis_file_basename
  bouf_nsis_file_basename="$(basename "${bouf_nsis_file_url}")"
  fetch-nsis

  echo '::group::Extract BOUF'
  mkdir -p "${RUNNER_TEMP}/bouf/bin"
  pushd "${RUNNER_TEMP}/bouf/bin" > /dev/null
  unzip "${RUNNER_TEMP}/${bouf_file_basename}"
  popd > /dev/null
  echo '::endgroup::'

  echo '::group::Extract BOUF NSIS components'
  mkdir -p "${RUNNER_TEMP}/bouf/nsis"
  pushd "${RUNNER_TEMP}/bouf/nsis" > /dev/null
  unzip "${RUNNER_TEMP}/${bouf_nsis_file_basename}"
  popd > /dev/null
  echo '::endgroup::'

  local bouf_location
  bouf_location="$(cygpath --windows "${RUNNER_TEMP}/bouf")"

  echo "bouf-location=${bouf_location}" >> "${GITHUB_OUTPUT}"
}

setup-bouf
