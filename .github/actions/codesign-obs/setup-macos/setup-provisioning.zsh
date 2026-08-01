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

setup-provisioning-profile() {
  if [[ -n "${PROVISIONING_PROFILE}" ]] {

    local profile_path="${RUNNER_TEMP}/build_profile.provisionprofile"
    base64 --decode --output=${profile_path} <<< "${PROVISIONING_PROFILE}"

    mkdir -p "${HOME}/Library/MobileDevice/Provisioning Profiles"
    security cms \
      -D \
      -i ${profile_path} \
      -o ${RUNNER_TEMP}/build_profile.plist

    local uuid
    uuid="$(plutil -extract UUID raw ${RUNNER_TEMP}/build_profile.plist)"
    print "::add-mask::${uuid}"

    local team_id
    team_id="$(plutil -extract TeamIdentifier.0 raw -expect string ${RUNNER_TEMP}/build_profile.plist)"
    print "::add-mask::${team_id}"

    if [[ ${team_id} != "${CODESIGN_TEAM:-}" ]] {
      print '::notice::Code Signing team in provisioning profile does not match certificate.'
    }

    cp ${profile_path} "${HOME}/Library/MobileDevice/Provisioning Profiles/${uuid}.provisionprofile"

    {
      print 'have-provisioning-profile=true'
      print "profile-uuid=${uuid}"
    }  >> ${GITHUB_OUTPUT}
    print 'Provisioning profile found and installed on runner.'
  } else {
    print 'have-provisioning-profile=false' >> ${GITHUB_OUTPUT}
    print 'No provisioning profile provided.'
  }
}

setup-provisioning-profile
