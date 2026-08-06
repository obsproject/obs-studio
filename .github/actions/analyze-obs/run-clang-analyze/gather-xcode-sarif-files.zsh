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

gather-xcode-sarif-files() {
  local -a analytics_files=(${ANALYTICS_PATH}/StaticAnalyzer/obs-studio/**/*.plist)

  local file
  for file (${analytics_files}) {
    mv ${file} ${ANALYTICS_PATH}/${${file:t}//plist/sarif}
  }
}

gather-xcode-sarif-files
