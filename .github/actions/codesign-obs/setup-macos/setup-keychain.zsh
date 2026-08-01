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

setup-keychain() {
  if [[ -n "${SIGNING_IDENTITY}" && \
        -n "${SIGNING_CERT}" && \
        -n "${SIGNING_CERT_PASSWORD}" ]] \
  {

    local certificate_path="${RUNNER_TEMP}/build_certificate.p12"
    local keychain_path="${RUNNER_TEMP}/app-signing.keychain-db"

    base64 --decode --output=${certificate_path} <<< "${SIGNING_CERT}"

    print '::group::Keychain setup'
    local keychain_password
    function {
      setopt NOXTRACE
      keychain_password="$(head --bytes=32 /dev/urandom | xxd -p -cols 0)"
      print "::add-mask::${keychain_password}"
    }

    security create-keychain -p ${keychain_password} ${keychain_path}
    security set-keychain-settings -lut 21600 ${keychain_path}
    security unlock-keychain -p ${keychain_password} ${keychain_path}

    security import ${certificate_path} \
      -P ${SIGNING_CERT_PASSWORD}  \
      -A \
      -t cert \
      -f pkcs12 \
      -k ${keychain_path} \
      -T /usr/bin/codesign -T /usr/bin/security -T /usr/bin/xcrun

    security set-key-partition-list \
      -S 'apple-tool:,apple:' \
      -k ${keychain_password} \
      ${keychain_path} &> /dev/null

    security list-keychain \
      -d user \
      -s ${keychain_path} \
      login-keychain
    print '::endgroup::'

    local team_id="${${SIGNING_IDENTITY##*\(}%%\)*}"
    print "::add-mask::${team_id}"

    rm ${certificate_path}

    {
      print 'have-codesign-ident=true'
      print "codesign-ident=${SIGNING_IDENTITY}"
      print "keychain-password=${keychain_password}"
      print "codesign-team=${team_id}"
    } >> ${GITHUB_OUTPUT}
    print 'Code signing Apple Developer certificate set up on runner.'
  } else {
    print 'have-codesign-ident=false' >> ${GITHUB_OUTPUT}
    print 'No code signing identity provided. No certificate was set up.'
  }
}

setup-keychain
