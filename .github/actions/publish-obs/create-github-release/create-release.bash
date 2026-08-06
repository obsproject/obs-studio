#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

check-files() {
  if [[ -n "${FILES:-}" ]]; then
    local output
    if ! output="$(jq --raw-output '.[]' <<< "${FILES}")"; then
      if [[ "${RUNNER_OS}" == 'Windows' ]]; then
        FILES="$(cygpath --unix "${FILES}")"
      fi

      if ! output="$(compgen -G "${FILES}")"; then
        echo "::warning::No files found with provided glob expression '${FILES}'."
        return 0
      fi
    fi

    local full_path
    while read -r file; do
      full_path="$(realpath "${file}")"

      if [[ -r "${full_path}" ]]; then
        release_files+=("${full_path}")
      else
        echo "::warning::Unmatched file '${file}' provided."
      fi
    done <<< "${output}"
  fi
}

setup-gh-arguments() {
  : "${RELEASE_TAG:="${GITHUB_REF}"}"
  tag_name="${RELEASE_TAG//refs\/tags\/}"

  gh_arguments+=(
    --title "${RELEASE_NAME:-"${tag_name}"}"
    --verify-tag
  )

  if [[ "${CREATE_DRAFT:-false}" == 'true' ]]; then
    gh_arguments+=(--draft)
  fi

  if [[ "${IS_PRERELEASE:-false}" == 'true' ]]; then
    gh_arguments+=(--prerelease --latest=false)
  fi
}

gh-edit-release() {
  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    RELEASE_NOTE_FILE="$(cygpath --unix "${RELEASE_NOTE_FILE:-}")"
  fi

  if [[ -n "${RELEASE_NOTE_FILE}" && -r "${RELEASE_NOTE_FILE}" ]]; then
    gh_arguments+=(--notes-file "${RELEASE_NOTE_FILE}")
  else
    gh_arguments+=(--notes "${RELEASE_NOTES:-}")
  fi

  gh release edit "${gh_arguments[@]}" "${tag_name}"
}

gh-create-release() {
  if [[ -n "${PREVIOUS_TAG:-}" && "${GENERATE_RELEASE_NOTES:-false}" == 'true' ]]; then
    gh_arguments+=(--generate-notes)
    echo "::notice::Generating release notes from provided previous tag '${PREVIOUS_TAG}'."
  else
    if [[ -n "${RELEASE_NOTE_FILE}" && -r "${RELEASE_NOTE_FILE}" ]]; then
      gh_arguments+=(--notes-file "${RELEASE_NOTE_FILE}")
    else
      gh_arguments+=(--notes "${RELEASE_NOTES:-}")
    fi
  fi

  gh release create "${gh_arguments[@]}" "${tag_name}"
}

generate-output() {
  gh_output="$(gh release view "${tag_name}" \
    --json id,url,uploadUrl,assets \
    --jq '{"id": .id, "url": .url, "uploadUrl": .uploadUrl, "assets": [.assets[].url]}')"

  local jq_output
  local release_id
  local release_url
  local release_upload_url
  jq_output="$(jq --raw-output '[.id, .url, .uploadUrl] | join(" ")' <<< "${gh_output}")"
  read -r release_id release_url release_upload_url <<< "${jq_output}"

  {
    echo "release-id=${release_id}"
    echo "release-url=${release_url}"
    echo "release-upload-url=${release_upload_url}"

    jq_output="$(jq --compact-output --monochrome-output --raw-output '.assets' <<< "${gh_output}")"
    if [[ -n "${jq_output}" ]]; then
      echo "release-asset-urls=${jq_output}"
    fi
  } >> "${GITHUB_OUTPUT}"
}

create-release() {
  if [[ -z "${RELEASE_TAG:-}" && "${GITHUB_REF}" != refs/tags/* && "${CREATE_DRAFT:-false}" != 'true' ]]; then
    echo "::error::Unable to create non-draft release with invalid tag ref '${GITHUB_REF}'."
    return 1
  fi

  local -a release_files=()
  check-files

  local tag_name
  local -a gh_arguments=()
  setup-gh-arguments

  local release_url
  local gh_output
  if gh release view "${tag_name}" &> /dev/null; then
    gh-edit-release
  else
    gh-create-release
  fi

  if (( ${#release_files[@]} )); then
    gh release upload --clobber "${tag_name}" "${release_files[@]}"
  fi

  generate-output
}

create-release
