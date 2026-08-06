#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

check-git-remote() {
  local remote_url
  remote_url="$(git config --local --get 'remote.origin.url')"

  local https_pattern='^https:\/\/([^\/]+)\/([^\/]+)\/(.+(\.git)?)$'
  if [[ "${remote_url}" =~ ${https_pattern} ]]; then
    hostname="${BASH_REMATCH[1]}"
    repository="${BASH_REMATCH[2]}/${BASH_REMATCH[3]}"
    protocol='https'
    return
  fi

  local ssh_pattern='git@([^:]+):(.+)\.git$'
  if [[ "${remote_url}" =~ ${ssh_pattern} ]]; then
    hostname="${BASH_REMATCH[1]}"
    repository="${BASH_REMATCH[2]}"
    protocol='ssh'
  fi
}

check-work-base() {
  if work_base_ref="$(git symbolic-ref --quiet --short HEAD)"; then
    work_base_type='branch'
  else
    if [[ -z "${BASE_REF}" ]]; then
      echo '::error::No source ref provided with checkout in detached HEAD mode.'
      return 1
    fi

    work_base_ref="$(git rev-parse HEAD)"
    work_base_type='commit'
  fi
}

setup-target-branch() {
  # Calculate fetch depth with some buffer based on number of new commits ahead of the PR branch
  local -i fetch_depth=10
  local commits_ahead
  commits_ahead="$(git rev-list --right-only --count "${base_ref}...${uuid}")"

  if (( commits_ahead )); then
    fetch_depth+="${commits_ahead}"
  fi

  # Try to fetch the target branch name from remote
  if ! git fetch \
    --no-tags \
    --no-recurse-submodules \
    --force \
    --depth="${fetch_depth}" \
    origin "${target_ref}:refs/remotes/origin/${target_ref}" &> /dev/null; \
  then
    # Target branch does not exist on remote, must be created locally first.
    # Use the temporary branch (by now either based off the main branch or the PR branch).
    git checkout -B "${target_ref}" "${uuid}" --

    # Check if the new target branch is actually ahead of the PR branch.
    commits_ahead="$(git rev-list --right-only --count "${base_ref}..${target_ref}")"

    if (( commits_ahead )); then
      outcome='new'
      deviated_from_base=1
    fi
  else
    # Target branch exists on remote, create a local checkout first.
    git checkout "${target_ref}" --

    # Check if the target branch is actually ahead of the local PR branch
    local target_branch_ahead
    target_branch_ahead="$(git rev-list --right-only --count "${base_ref}..${target_ref}")"

    # Logic adapted from https://github.com/peter-evans/create-pull-request
    # The target branch will be reset under these conditions:
    # * The target branch has changes not contained in the temporary branch
    # * If the branches have no diverting change-set, but the amount of commits differs
    #   (e.g. the same changes now encompass 5 instead of 7 commits).
    # * If the temporary branch is behind the target branch
    # * Finally, if the branch has the same amount of commits, and no superficial changes,
    #   compare the actual change-set. If the changes are not equal, the base has deviated
    #   from the target.
    if ! git diff --quiet "${target_ref}..${uuid}" \
      || (( target_branch_ahead != commits_ahead )) \
      || (( commits_ahead <= 0 )); then
        local diff_target
        local diff_temp

        if diff_target="$(git diff --stat "${target_ref}..${target_ref}~${commits_ahead}" -- 2> /dev/null)" \
          && diff_temp="$(git diff --stat "${uuid}..${uuid}~${commits_ahead}" -- 2> /dev/null)"; then
            if [[ "${diff_target}" != "${diff_temp}" ]]; then
              # Reset target branch to temporary branch.
              git checkout -B "${target_ref}" "${uuid}" --
            fi
        fi
    fi

    # Check if local target branch deviates from remote.
    local target_ahead
    local target_behind
    target_ahead="$(git rev-list --right-only --count "origin/${target_ref}..${target_ref}")"
    target_behind="$(git rev-list --left-only --count "origin/${target_ref}..${target_ref}")"

    if (( target_ahead == 0 && target_behind == 0 )); then
      outcome='even'
    else
      outcome='deviated'
    fi

    # Check if target branch is still ahead of base branch
    commits_ahead="$(git rev-list --right-only --count "${base_ref}..${target_ref}")"

    if (( commits_ahead )); then
      deviated_from_base=1
    fi
  fi
}

setup-pull-branch() {
  local uuid
  uuid="$(uuidgen)"

  # Create temporary branch for local changes
  git checkout -B "${uuid}" HEAD --

  # Check if there are any uncommitted changes
  local changes
  changes="$(git status --porcelain --untracked-files=normal --)"

  # Commit all changes
  if [[ -n "${changes}" ]]; then
    git add --all
    git \
      -c "author.name=${AUTHOR% *}" \
      -c "author.email=${AUTHOR##* }" \
      -c "committer.name=${COMMITTER%% *}" \
      -c "committer.email=${COMMITTER##* }" \
      commit \
      --message="${COMMIT_MESSAGE}"
  fi

  # Stash anything else
  local -i has_stash=0
  local stash_result
  stash_result="$(LC_ALL=C git stash push --include-untracked)"
  if [[ "${stash_result}" != 'No local changes to save' ]]; then
    has_stash=1
  fi

  # Reset current working base branch
  if [[ "${work_base_type}" == 'branch' ]]; then
    git checkout "${work_base_ref}" --
    git reset --hard "origin/${work_base_ref}"
  fi

  # If PR branch should not be based on working base branch (e.g. not on the main branch),
  # rebase the temporary branch on the PR branch.
  if [[ "${work_base_ref}" != "${base_ref}" ]]; then
    # Check out the PR branch as base branch
    git fetch --no-tags --no-recurse-submodules --force --depth=1 origin "${base_ref}:${base_ref}"
    git checkout "${base_ref}" --

    # Get all changes between working base branch and temporary branch
    local commits
    commits="$(git rev-list --reverse "${work_base_ref}..${uuid}" .)"

    # Cherry-pick all changes
    local commit
    while read -r commit; do
      git cherry-pick --strategy=recursive --strategy-option=theirs "${commit}"
    done <<< "${commits}"

    # Reset temporary branch to new HEAD based on the PR branch
    git checkout -B "${uuid}" HEAD --

    # Reset the PR branch
    git fetch --no-tags --no-recurse-submodules --force --depth=1 origin "${base_ref}:${base_ref}"
  fi

  setup-target-branch

  base_ref_sha="$(git rev-parse "${base_ref}")"
  head_ref_sha="$(git rev-parse "${target_ref}")"

  # Clean up the temporary branch
  git branch --delete --force "${uuid}"

  # Check out working base directory to reset git state
  git checkout "${work_base_ref}"

  if (( has_stash )); then
    git stash pop
  fi
}


prepare-changes() {
  if [[ "${RUNNER_OS}" == 'Linux' ]] && ! command -v uuidgen > /dev/null; then
    sudo apt-get update
    sudo apt-get install uuid-runtime
  fi

  local hostname
  local protocol
  local repository
  check-git-remote

  : "${repository}"

  if [[ "${protocol}" == 'https' ]]; then
    local base64_string
    {
      set +x > /dev/null
      base64_string="$(printf '%s' "x-access-token:${GH_TOKEN}" | base64 -w 0)"
      echo "::add-mask::${base64_string}"

      if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi
    }
    git config --local "http.https://${hostname}/.extraheader" "AUTHORIZATION: basic ${base64_string}"
  fi

  local work_base_ref
  local work_base_type
  check-work-base

  local base_ref="${BASE_REF:-"${work_base_ref}"}"
  local target_ref="${BRANCH}"

  if [[ "${base_ref}" == "${target_ref}" ]]; then
    echo '::error::Base branch and target branch cannot be identical.'
    return 1
  fi

  local outcome='none'
  local -i deviated_from_base=0
  local base_ref_sha
  local head_ref_sha
  setup-pull-branch

  {
    echo "outcome=${outcome}"
    echo "base-ref-sha=${base_ref_sha}"
    echo "head-ref-sha=${head_ref_sha}"

    if (( deviated_from_base )); then
      echo 'deviated-from-base=true'
    else
      echo 'deviated-from-base=false'
    fi
  } >> "${GITHUB_OUTPUT}"
}

prepare-changes
