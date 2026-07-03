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

adjust-appcast() {
  local -a appcasts=(${RUNNER_TEMP}/appcasts/*_v2.xml(N))

  if (( ! ${#appcasts} )) {
    print '::error::No appcasts found.'
    return 1
  }

  local appcast
  local adjusted
  for appcast (${appcasts}) {
    adjusted="${appcast//.xml/-adjusted.xml}"

    xsltproc \
      --stringparam pDeltaUrl "${FEED_URL:h}/sparkle_deltas/${ARCHITECTURE}/" \
      --stringparam pSparkleUrl "${FEED_URL:h}" \
      --stringparam pCustomTitle "${CUSTOM_TITLE:-}" \
      --stringparam pCustomLink "${CUSTOM_LINK:-}" \
      --output ${adjusted} \
      ${GITHUB_ACTION_PATH}/appcast_adjust.xslt \
      ${appcast}

    xmllint --format ${adjusted} >! ${appcast}
    rm ${adjusted}
  }
}

adjust-appcast
