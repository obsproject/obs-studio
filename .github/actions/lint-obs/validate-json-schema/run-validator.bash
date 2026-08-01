#!/usr/bin/env bash
# shellcheck disable=SC2154,SC1091

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

shopt -s extglob

run-validator() {

  local output
  if ! output="$(jq --raw-output '.[]' 2> /dev/null <<< "${JSON_FILES}")"; then
    if ! output="$(compgen -G "${JSON_FILES}")"; then
      echo "::warning::No JSON files found with provided glob expression '${JSON_FILES}'."
      return 0
    fi
  fi

  local -a file_list=()
  while read -r file; do
    file_list+=("${file}")
  done <<< "${output}"

  if (( ! ${#file_list[@]} )); then
    echo '::error::No json files found to validate.'
    return 1
  fi

  source "${RUNNER_TEMP}/compatibility-validator-venv/bin/activate"
  declare -x PYTHONUNBUFFERED=1
  python3 "${GITHUB_ACTION_PATH}/check-jsonschema.py" \
    --loglevel INFO \
    --output "${RUNNER_TEMP}/validation_errors.json" \
    "${file_list[@]}"

  deactivate
}

run-validator
