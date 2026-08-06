#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-cng() {
  mkdir -p "${RUNNER_TEMP}/google-cng"
  local repo_name="GoogleCloudPlatform/kms-integrations"

  cd "${RUNNER_TEMP}/google-cng"

  gh release download "cng-v${CNG_VERSION}" --repo "${repo_name}" --pattern "*amd64.zip"

  local release_name="kmscng-${CNG_VERSION}-windows-amd64.zip"

  if [[ ! -r "${release_name}" ]]; then
    echo "::error::No CNG provider release found - expected '${release_name}'."
    return 1
  fi

  unzip -j "${release_name}"

  local -a openssl_arguments=(
    -sha384
    --verify
    "${GITHUB_ACTION_PATH}/cng-release-signing-key.pem"
    -signature kmscng.msi.sig
  )

  openssl dgst "${openssl_arguments[@]}" kmscng.msi
  msiexec //qn //norestart //i kmscng.msi

  local windows_path
  windows_path="$(cygpath --windows "${RUNNER_TEMP}/google-cng")"
  echo "google-cng-path=${windows_path}" >> "${GITHUB_OUTPUT}"
}

setup-cng
