#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

select-git-ref() {
  # 4b825dc642cb6eb9a060e54bf8d69288fbee4904 is a "hidden" sha1 hash of
  # the "empty tree", retrieved via 'git hash-object -t tree /dev/null',
  # and used here as a last-resort fallback to always provide a valid
  # git ref.

  local empty_tree_hash
  empty_tree_hash="$(git hash-object -t tree /dev/null)"

  echo '::group::Checking for compatible git ref'
  # If base ref is provided, check if it represents a valid object in the source tree. If enabled,
  # use 'HEAD~1' (the commit before the current one) to compare against.
  if [[ -n "${GIT_BASE_REF:-}" ]]; then
    if ! git cat-file -e "${GIT_BASE_REF}" &> /dev/null; then
      if [[ "${USE_FALLBACK:-false}" == 'true' ]]; then
        echo "::warning::Provided base reference '${GIT_BASE_REF}' is invalid. Using 'HEAD~1' instead."
        GIT_BASE_REF='HEAD~1'
      else
        echo "::error::Provided base reference '${GIT_BASE_REF}' is invalid."
        return 1
      fi
    fi
  else
    # If base ref is not provided, check if the SHA of the most recent commit on ref before the current
    # push is a valid object. Use the fallback "empty tree" hash otherwise. This will effectively list
    # all changes since the beginning of the repository.
    if ! git cat-file -e "${GITHUB_REF_BEFORE:-}" &> /dev/null; then
      GITHUB_REF_BEFORE="${empty_tree_hash}"
    fi

    # Start with the commit before the current one as a baseline.
    GIT_BASE_REF='HEAD~1'
    case "${GITHUB_EVENT_NAME:-}" in
      pull_request)
        # Use the target branch of the pull request to compare against.
        echo "Using pull request target branch '${GITHUB_BASE_REF}'."
        GIT_BASE_REF="origin/${GITHUB_BASE_REF}"

        if ! git show-ref --exists "${GIT_BASE_REF}" &> /dev/null; then
          git fetch origin
        fi
        ;;
      push)
        # Use the SHA of the most recent commit on ref before the current
        # push to compare against.
        if [[ "${GITHUB_EVENT_FORCED:-false}" != 'true' ]]; then
          echo "Normal push detected. Using most recent ref before current push '${GITHUB_REF_BEFORE}'."
          GIT_BASE_REF="${GITHUB_REF_BEFORE}"
        else
          echo "Force push detected. Using ref before current one 'HEAD~1'."
        fi
        ;;
      *) ;;
    esac
  fi
  echo '::endgroup::'
}

check-changes() {
  select-git-ref

  local -a path_spec
  read -a path_spec -r <<< "${PATH_SPEC:-.}"

  local diff_content
  diff_content="$(git diff \
    --name-only \
    --diff-filter="${DIFF_FILTER:-}" \
    "${GIT_BASE_REF}" \
    "${GIT_REF}" \
    -- \
    "${path_spec[@]}")"

  local full_path
  local -a changes_absolute=()
  local -a changes_relative=()

  while read -r file; do
    if [[ -n "${file}" ]]; then
      full_path="$(realpath "${file}")"
      if [[ "${RUNNER_OS}" == 'Windows' ]]; then
        full_path="$(cygpath --windows "${full_path}")"
      fi
      changes_absolute+=("${full_path}")
      changes_relative+=("${file}")
    fi
  done <<< "${diff_content}"

  {
    if (( ${#changes_absolute[@]} )); then
      local json_changes_absolute
      json_changes_absolute="$(jq --compact-output --monochrome-output --raw-input '
        .,inputs | split(" ")
      ' <<< "${changes_absolute[@]}")"

      local json_changes_relative
      json_changes_relative="$(jq --compact-output --monochrome-output --raw-input '
        .,inputs | split(" ")
      ' <<< "${changes_relative[@]}")"

      echo "has-changed-files=true"
      echo "changed-files=${json_changes_absolute}"
      echo "changed-repo-files=${json_changes_relative}"
    else
      echo "has-changed-files=false"
      echo "changed-files=[]"
      echo "changed-repo-files=[]"
    fi
  } >> "${GITHUB_OUTPUT}"
}

check-changes
