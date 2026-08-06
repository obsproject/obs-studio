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

generate-appcast() {
  print -n "${SIGNING_KEY}" >! ${RUNNER_TEMP}/sparkle.key

  local release_notes_url="${FEED_URL//updates_*/notes_${CHANNEL}.html}"

  local -a sparkle_options=(
    --ed-key-file ${RUNNER_TEMP}/sparkle.key
    --download-url-prefix "${FEED_URL:h}"
    --full-release-notes-url ${release_notes_url}
    --maximum-versions 0
    --maximum-deltas ${DELTA_COUNT}
    --channel ${CHANNEL}
  )

  if (( ${+RUNNER_DEBUG} )) {
    sparkle_options+=(--verbose)
  }

  if [[ ${CHANNEL} == 'stable' && -n ${ROLLOUT_INTERVAL} ]] {
    local -i interval=0
    local -a match=()
    local mbegin
    local mend
    if [[ ${ROLLOUT_INTERVAL} == (#b)(<->##) ]] {
      interval=${match[1]}

      if (( interval < 60 || interval > 604800 )) {
        print "::error::Unsupported rollout interval '${interval} (supported range: [60-604800])."
        return 1
      }
    } else {
      print "::error::Provided rollout interval value '${ROLLOUT_INTERVAL}' is not a number."
      return 1
    }

    sparkle_options+=(--phased-rollout-interval ${interval})
  }

  if [[ ! -d ${SPARKLE_PATH} ]] {
    print "::error::No Sparkle distribution found at '${SPARKLE_PATH}'"
    return 1
  }

  ${SPARKLE_PATH}/bin/generate_appcast \
    ${sparkle_options} \
    ${RUNNER_TEMP}/appcast_builds

  local -a deltas=(${RUNNER_TEMP}/appcast_builds/*.delta(N))

  if (( ${#deltas} )) {
    local delta_destination="${RUNNER_TEMP}/appcasts/deltas/${ARCHITECTURE}"
    mkdir -p ${delta_destination}
    mv ${deltas} ${delta_destination}
  }

  mv ${RUNNER_TEMP}/appcast_builds/*.xml ${RUNNER_TEMP}/appcasts
}

generate-appcast
