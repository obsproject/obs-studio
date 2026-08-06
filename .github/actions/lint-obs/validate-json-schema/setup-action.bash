#!/usr/bin/env bash
# shellcheck disable=SC2154,SC1091

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-action() {
  echo '::group::Python Setup'
  case "${RUNNER_OS}" in
    Linux)
      sudo apt-get update
      sudo apt-get install --yes --no-install-recommends python3
      ;;
    macOS)
      brew update
      brew install ${RUNNER_DEBUG:+--verbose} python3
      ;;
    *)
      echo "::error::Unsupported runner operating system '${RUNNER_OS}'."
      return 1
      ;;
  esac

  python3 -m venv "${RUNNER_TEMP}/compatibility-validator-venv"
  echo '::endgroup::'

  echo '::group::Install Action dependencies'
  source "${RUNNER_TEMP}/compatibility-validator-venv/bin/activate"
  python3 -m pip install --upgrade jsonschema json_source_map
  echo '::endgroup::'

  deactivate
}

setup-action
