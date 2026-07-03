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
if (( ${+RUNNER_DEBUG} )) setopt XTRACE

: ${CI:?}

create-legacy-appcast() {
  local -a appcasts=(${RUNNER_TEMP}/appcasts/*_v2.xml)

  if (( ! ${#appcasts} )) {
    print '::error::No appcasts found.'
    return 1
  }

  mkdir -p "${RUNNER_TEMP}/appcasts/stable"

  local appcast
  local legacy
  for appcast (${appcasts}) {
    legacy="${appcast//.xml/-legacy.xml}"
    xsltproc \
      --output ${legacy} \
      ${GITHUB_ACTION_PATH}/appcast_legacy.xslt \
      ${appcast}

    xmllint --format ${legacy} >! ${RUNNER_TEMP}/appcasts/stable/${${appcast:t}//_v2.xml/.xml}

    if [[ ${ARCHITECTURE} == x86_64 ]] {
      xmllint --format ${legacy} >! ${RUNNER_TEMP}/appcasts/stable/${${appcast:t}//_x86_64_v2.xml/.xml}
    }

    rm ${legacy}
  }
}

create-legacy-appcast
