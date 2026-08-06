#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-brew() {
  local brew_env
  brew_env="$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"
  eval "${brew_env}"
  brew update

  echo "/home/linuxbrew/.linuxbrew/bin:/home/linuxbrew/.linuxbrew/sbin" >> "${GITHUB_PATH}"
}

setup-ubuntu() {
  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    echo '::group::Brew configuration'
    brew config
    echo '::endgroup::'
  fi

  echo "::group::Installing ${LINTER_COMMAND:-}..."
  case "${LINTER_COMMAND:-}" in
    clang-format)
      setup-brew
      brew trust obsproject/tools
      brew install ${RUNNER_DEBUG:+--verbose} obsproject/tools/clang-format@22
      echo "/home/linuxbrew/.linuxbrew/opt/clang-format@22/bin" >> "${GITHUB_PATH}"
      ;;
    gersemi)
      setup-brew
      brew install ${RUNNER_DEBUG:+--verbose} gersemi
      ;;
    swift-format)
      setup-brew
      brew install ${RUNNER_DEBUG:+--verbose} swift-format
      ;;
    xmllint)
        sudo apt-get update
        sudo apt-get install --no-install-recommends --yes libxml2-utils
      ;;
    zizmor)
      setup-brew
      brew install ${RUNNER_DEBUG:+--verbose} zizmor
      ;;
    *)
      return 1
  esac
  echo '::endgroup::'
}

setup-ubuntu
