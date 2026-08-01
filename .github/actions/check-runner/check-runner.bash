#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

check-runner() {
  local error_message

  local -i has_supported_os=0
  local os_candidate
  while read -r os_candidate; do
    if [[ "${RUNNER_OS}" == "${os_candidate}" ]]; then
      has_supported_os=1
    fi
  done <<< "${CHECK_OS}"

  if (( ! has_supported_os )); then
    error_message="Unsupported runner operating system '${RUNNER_OS}'."
    if [[ -n "${CUSTOM_ERROR:-}" ]]; then
      error_message="${CUSTOM_ERROR}"
    fi

    echo "::error::${error_message}"
    return 1
  fi
}

check-runner
