#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

check-version-tag() {
  local version_regex='^(([0-9]+)\.([0-9]+)\.([0-9]+))(-(rc|beta)([0-9]+))?$'

  if [[ "${GIT_REF}" =~ ${version_regex} ]]; then
    {
      echo "version=${BASH_REMATCH[0]}"
      echo "major=${BASH_REMATCH[2]}"
      echo "minor=${BASH_REMATCH[3]}"
      echo "patch=${BASH_REMATCH[4]}"
    } >> "${GITHUB_OUTPUT}"

    if [[ -n "${BASH_REMATCH[5]}" ]]; then
      {
        echo "pre-release=${BASH_REMATCH[5]}"
        echo "number=${BASH_REMATCH[7]}"
      } >> "${GITHUB_OUTPUT}"
      echo "Semantic pre-release version detected for ${GIT_REF}."
    else
      echo "Semantic version detected for ${GIT_REF}."
    fi

    echo "is-valid-semver=true"
    return 0
  fi

  echo "is-valid-semver=false" >> "${GITHUB_OUTPUT}"
  local error_message="No semantic version detected for ${GIT_REF}."
  if [[ "${FAIL_ON_MISMATCH:-false}" == 'true' ]]; then
    echo "::error::${error_message}"
    return 1
  else
    echo "${error_message}"
  fi
}

check-version-tag
