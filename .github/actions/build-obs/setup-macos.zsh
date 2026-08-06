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

switch-xcode-version() {
  if [[ -n ${XCODE_VERSION:-} ]] {
    if [[ ${XCODE_VERSION} == <->##.<-> ]] {
      if [[ -d /Applications/Xcode_${XCODE_VERSION}.app/Contents/Developer ]] {
        sudo xcode-select --switch /Applications/Xcode_${XCODE_VERSION}.app/Contents/Developer
      } else {
        print "::error::Xcode version ${XCODE_VERSION} not found on runner."
        return 1
      }
    } else {
      print "::error::Provided invalid version value ${XCODE_VERSION}."
      return 1
    }
  } else {
    sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
  }
}

setup-macos() {
  switch-xcode-version

  local cache_path="${XCODE_CAS_PATH:-"${HOME}/Library/Developer/Xcode/DerivedData/CompilationCache.noindex"}"

  if [[ ! -d ${cache_path} ]] {
    mkdir -p ${cache_path}
  }

  print "xcode-cas-path=${cache_path}" >> ${GITHUB_OUTPUT}
}

setup-macos
