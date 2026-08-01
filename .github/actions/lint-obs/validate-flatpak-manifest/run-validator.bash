#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

run-validator() {
  if [[ "${FAIL_MODE:-never}" == 'never' ]]; then set +o errexit; fi

  if [[ ! -r "${MANIFEST_FILE}" ]]; then
    echo "::error:: Manifest file ${MANIFEST_FILE} not found."
    return 1
  fi

  local full_path
  full_path="$(realpath "${MANIFEST_FILE}")"

  declare -x PYTHONUNBUFFERED=1
  python3 "${PWD}/build-aux/format-manifest.py" \
    "${full_path}" \
    --check \
    --loglevel INFO
}

run-validator
