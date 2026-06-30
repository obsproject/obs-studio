#!/usr/bin/env bash
# shellcheck disable=SC2154,SC1091

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

generate-documentation() {
  local checkout="${PWD}"
  local docs_dir="${checkout}/docs/sphinx"
  if ! [[ -r "${docs_dir}/Makefile" && -r "${docs_dir}/conf.py" ]]; then
    echo '::error::Sphinx Makefile and conf.py not found.'
    return 1
  fi

  mkdir -p "${RUNNER_TEMP}/sphinx_build"

  source "${RUNNER_TEMP}/generate-docs-venv/bin/activate"

  local -a build_arguments=(
    "${docs_dir}"
    "${RUNNER_TEMP}/sphinx_build"
    --jobs auto
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    build_arguments+=(--verbose)
  fi

  SPHINX_ENV="production" sphinx-build "${build_arguments[@]}"
}

generate-documentation
