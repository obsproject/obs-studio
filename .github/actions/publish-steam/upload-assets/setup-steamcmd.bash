#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

setup-steamcmd() {
  local -A download_urls=(
    [windows]='https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip'
    [macos]='https://steamcdn-a.akamaihd.net/client/installer/steamcmd_osx.tar.gz'
    [linux]='https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz'
  )

  local download_url="${download_urls["${RUNNER_OS,,*}"]}"
  local file_name
  file_name="$(basename "${download_url}")"

  echo '::group::Downloading Steam command-line client...'
  curl \
    --location \
    --output-dir "${RUNNER_TEMP}" \
    --remote-name \
    -- "${download_url}"
  echo '::endgroup::'

  mkdir -p "${RUNNER_TEMP}/steamcmd"
  pushd "${RUNNER_TEMP}/steamcmd" > /dev/null

  case "${RUNNER_OS,,*}" in
  windows)
    echo '::group::Extract steamcmd archive...'
    unzip "${RUNNER_TEMP}/${file_name}"
    echo '::endgroup::'

    echo '::group::Run steamcmd self-update...'
    local -i return_code=0
    "${PWD}/steamcmd.exe" +quit || return_code="${?}"
    echo '::endgroup::'

    if (( return_code != 0 && return_code != 7  )); then
      echo '::error::Unexpected exit code for first run of steamcmd on Windows.'
      return 1
    fi
    ;;
  macos)
    echo '::group::Extract steamcmd archive...'
    tar --extract --verbose --file "${RUNNER_TEMP}/${file_name}"
    echo '::endgroup::'

    echo '::group::Run steamcmd self-update...'
    bash "${PWD}/steamcmd.sh" +quit
    echo '::endgroup::'
    ;;
  linux)
    echo '::group::Extract steamcmd archive...'
    tar --extract --verbose --file "${RUNNER_TEMP}/${file_name}"
    echo '::endgroup::'

    echo '::group::Install Linux system dependencies...'
    sudo apt-get update
    sudo apt-get install --yes --no-install-recommends lib32gcc-s1
    echo '::endgroup::'

    echo '::group::Run steamcmd self-update...'
    bash "${PWD}/steamcmd.sh" +quit
    echo '::endgroup::'
    ;;
  *)
    echo "::error::Unsupported runner operating system '${RUNNER_OS}'."
    return 1
    ;;
  esac
  popd > /dev/null

  local steamcmd_path="${RUNNER_TEMP}/steamcmd"
  if [[ "${RUNNER_OS}" == 'Windows' ]]; then
    steamcmd_path="$(cygpath --windows "${steamcmd_path}")"
  fi

  echo "path=${steamcmd_path}" >> "${GITHUB_OUTPUT}"
}

setup-steamcmd
