#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

shopt -s extglob

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-branches() {
  local branch_name
  local description
  local is_prerelease='false'

  case "${GITHUB_EVENT_NAME}" in
  release|workflow_dispatch)
    if [[ "${GITHUB_REF_TYPE}" == 'tag' ]]; then
      local version="${GIT_REF//refs\/tags\/}"
      local version_regex='^(([0-9]+)\.([0-9]+)\.([0-9]+))(-(rc|beta)([0-9]+))?$'

      if [[ "${version}" =~ ${version_regex} ]]; then
        local gh_output
        gh_output="$(gh release view "${version}" --json isPrerelease,tagName --jq 'join(" ")')"
        read -r is_prerelease description <<< "${gh_output}"

        description="${version}"

        if [[ "${is_prerelease:-false}" == 'true' ]]; then
          branch_name='beta_staging'
        else
          branch_name='staging'
        fi
      else
          echo "::error::Invalid git ref '${GIT_REF}' provided."
          return 1
      fi
    else
      description="g${GIT_REF}"
      is_prerelease='false'
      branch_name='nightly'
    fi
    ;;
    schedule)
      description="g${GIT_REF}"
      is_prerelease='false'
      branch_name='nightly'
      ;;
    *)
      echo "::error::Unsupported GitHub workflow event '${GITHUB_EVENT_NAME}'."
      return 1
      ;;
  esac

  {
    echo "branch-name=${branch_name}"
    echo "description=${description}"
    echo "is-prerelease=${is_prerelease}"
  } >> "${GITHUB_OUTPUT}"
}

setup-branches
