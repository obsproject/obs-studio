#!/usr/bin/env zsh

builtin emulate -L zsh
setopt ERR_EXIT
setopt ERR_RETURN
setopt EXTENDED_GLOB
setopt FUNCTION_ARGZERO
setopt NO_AUTO_PUSHD
setopt NO_PUSHD_IGNORE_DUPS
setopt NO_UNSET
setopt PIPE_FAIL
setopt PUSHD_SILENT
setopt WARN_CREATE_GLOBAL
setopt WARN_NESTED_VAR

: ${CI:?}
if (( ${+RUNNER_DEBUG} )) setopt XTRACE

setup-macos() {
  if (( ${+RUNNER_DEBUG} )) {
    print '::group::Brew configuration'
    brew config
    print '::endgroup::'
  }

  echo "::group::Installing ${LINTER_COMMAND:-}..."
  case ${LINTER_COMMAND:-} {
    clang-format)
      brew update
      brew trust obsproject/tools/clang-format@22
      brew install ${RUNNER_DEBUG:+--verbose} obsproject/tools/clang-format@22
      ;;
    gersemi)
      brew update
      brew install ${RUNNER_DEBUG:+--verbose} gersemi
      ;;
    swift-format)
      brew update
      brew install ${RUNNER_DEBUG:+--verbose} swift-format
      ;;
    xmllint) ;;
    zizmor)
      brew update
      brew install ${RUNNER_DEBUG:+--verbose} zizmor
      ;;
    *)
      return 1
  }
  echo "::endgroup::"
}

setup-macos
