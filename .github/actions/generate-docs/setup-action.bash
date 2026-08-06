#!/usr/bin/env bash
# shellcheck disable=SC2154,SC1091

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

fixup-documentation() {
  # This expression will first try to match all the LIBOBS_API_[...]_VER
  # lines in 'obs-config.h', before removing the '#define ' prefix and
  # trimming away whitespace characters and linebreaks.
  # This will yield a single line with the each major, minor, and patch
  # version variable name followed by its value, so every even item
  # in this string will contain a version part.
  local version_lines
  version_lines="$(grep \
    --extended-regexp \
    --regexp="#define LIBOBS_API_(MAJOR|MINOR|PATCH)_VER *" "${checkout}/libobs/obs-config.h" \
      | sed 's/#define //g' \
      | tr -s ' ' \
      | tr '\n' ' ')"

  local major
  local minor
  local patch
  read -r _ major _ minor _ patch _ <<< "${version_lines}"

  echo "Detected 'libobs' API version: ${major}.${minor}.${patch}."

  local current_year
  current_year="$(date +"%Y")"

  # This expression simply replaces the definition of the 'version' and
  # 'release' variables in the Python script with updated variants using
  # the version tokens set by the previous expression.
  # The copyright variable assignment is updated to use the current
  # local year as the second year value.
  sed -i -E \
    -e "s/version = '([0-9]+\.[0-9]+\.[0-9]+)'/version = '${major}.${minor}.${patch}'/g" \
    -e "s/release = '([0-9]+\.[0-9]+\.[0-9]+)'/release = '${major}.${minor}.${patch}'/g" \
    -e "s/copyright = '(2017-[0-9]+, Lain Bailey)'/copyright = '2017-${current_year}, Lain Bailey'/g" \
    "${checkout}/docs/sphinx/conf.py"

  if [[ "${DISABLE_LINK_EXTENSIONS:-false}" == 'true' ]]; then
    sed -i -e "s/html_link_suffix = None/html_link_suffix = ''/g" \
     "${checkout}/docs/sphinx/conf.py"
    echo "artifact-name=obs-studio-documentation-without-extensions-${GITHUB_SHA:0:9}" >> "${GITHUB_OUTPUT}"
  else
    echo "artifact-name=obs-studio-documentation-with-extensions-${GITHUB_SHA:0:9}" >> "${GITHUB_OUTPUT}"
  fi
}

setup-action() {
  local checkout="${PWD}"
  if ! [[ -d "${checkout}/.git" && -d "${checkout}/docs/sphinx" ]]; then
    echo '::error::Action needs to be run from an obs-studio checkout root directory'
    return 1
  fi

  local docs_dir="${checkout}/docs/sphinx"

  echo '::group::Python Setup'
  case "${RUNNER_OS}" in
    Linux)
      sudo apt-get update
      sudo apt-get install --yes --no-install-recommends python3
      ;;
    macOS)
      brew update
      brew install ${RUNNER_DEBUG:+--verbose} python3
      ;;
    *)
      echo "::error::Unsupported runner operating system '${RUNNER_OS}'."
      return 1
      ;;
  esac

  python3 -m venv "${RUNNER_TEMP}/generate-docs-venv"
  echo '::endgroup::'

  echo '::group::Sphinx Setup'
  source "${RUNNER_TEMP}/generate-docs-venv/bin/activate"
  python3 -m pip install --upgrade sphinx poetry

  python3 -m pip install --requirement "${docs_dir}/requirements.txt"
  echo '::endgroup::'

  fixup-documentation

  deactivate
}

setup-action
