#!/usr/bin/env zsh

builtin emulate -L zsh
setopt PUSHD_SILENT
setopt EXTENDED_GLOB
setopt ERR_EXIT
setopt ERR_RETURN
setopt NO_UNSET
setopt PIPE_FAIL
setopt NO_AUTO_PUSHD
setopt NO_PUSHD_IGNORE_DUPS
setopt NO_GLOB_SUBST
setopt WARN_CREATE_GLOBAL
setopt WARN_NESTED_VAR

: ${CI:?}
if (( ${+RUNNER_DEBUG} )) setopt XTRACE

setup-sparkle() {
  local sparkle_url="${SPARKLE_URL}/${SPARKLE_VERSION}/Sparkle-${SPARKLE_VERSION}.tar.xz"

  local url_regex='^https:\/\/.+\/[0-9\.]+\/Sparkle-[0-9\.]+\.tar\.xz$'

  if ! [[ ${sparkle_url} =~ ${url_regex} ]] {
    print "::error::Invalid Sparkle download url: '${sparkle_url}'"
    return 1
  }

  curl \
    --location \
    --remote-name \
    --output-dir ${RUNNER_TEMP} \
    -- ${sparkle_url}

  local shasum_result="$(shasum --algorithm 256 ${RUNNER_TEMP}/${sparkle_url:t})"
  local -a checksum
  read -r -A checksum <<< "${shasum_result}"

  if [[ ${SPARKLE_CHECKSUM} != ${checksum[1]} ]] {
    print "::error::${sparkle_url:t} checksum mismatch: ${checksum[1]} (expected: ${SPARKLE_CHECKSUM})."
    return 1
  }

  mkdir -p ${RUNNER_TEMP}/Sparkle

  tar \
    --extract \
    --verbose \
    --xz \
    --file ${RUNNER_TEMP}/${sparkle_url:t} \
    --directory ${RUNNER_TEMP}/Sparkle

  print "sparkle-location=${RUNNER_TEMP}/Sparkle" >> ${GITHUB_OUTPUT}
}

setup-sparkle
