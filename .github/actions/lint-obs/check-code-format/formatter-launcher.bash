#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

formatter-launcher() {
  local checkout="${PWD}"

  if [[ "${FAIL_MODE:-never}" == 'never' ]]; then set +e; fi

  local output
  if ! output="$(jq --raw-output '.[]' 2> /dev/null <<< "${CHANGED_FILES}")"; then
    if ! output="$(compgen -G "${CHANGED_FILES}")"; then
      echo "::warning::No changed files found using glob expression '${CHANGED_FILES}'."
      return 0
    fi
  fi

  local change
  local -a changes=()
  while read -r change; do
    changes+=("${change}")
  done <<< "${output}"

  local launcher
  if [[ "${RUNNER_OS}" == 'macOS' ]]; then
    launcher="${checkout}/build-aux/.run-format.zsh"
  else
    launcher="${checkout}/build-aux/.run-format.bash"
  fi

  ${launcher} \
    --linter "${LINTER_COMMAND}" \
    --check \
    --github \
    "${changes[@]}"
}

formatter-launcher
