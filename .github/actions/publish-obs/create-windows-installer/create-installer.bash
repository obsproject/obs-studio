#!/usr/bin/env bash
# shellcheck disable=SC2154,SC1091

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

create-installer() {
  declare pandoc_path
  pandoc_path="$(cygpath --unix "${LOCALAPPDATA}/Programs/Pandoc")"
  export PATH="${pandoc_path}:${PATH}"
  BOUF_PATH="$(cygpath --unix "${BOUF_PATH}")"

  mkdir -p "${RUNNER_TEMP}/previous"

  local -a bouf_args=(
    --config "${GITHUB_ACTION_PATH}/config_${ARCHITECTURE//x86_64/x64}.toml"
    --version "${GITHUB_REF_NAME}"
    --input "${RUNNER_TEMP}/obs-installer-build"
    --previous "${RUNNER_TEMP}/previous"
    --output "${RUNNER_TEMP}/output"
    --packaging-only
  )

  if [[ -n "${RUNNER_DEBUG:-}" ]]; then
    bouf_args+=(--verbose)
  fi

  "${BOUF_PATH}/bin/bouf.exe" "${bouf_args[@]}"

  local -a installer_files=("${RUNNER_TEMP}"/output/*.exe)
  local installer_path="${installer_files[0]}"
  local -a pdb_archive_files=("${RUNNER_TEMP}"/output/*-"${ARCHITECTURE}"-pdbs.zip)
  local pdb_archive_path="${pdb_archive_files[0]}"
  local -a archive_files=("${RUNNER_TEMP}"/output/*-"${ARCHITECTURE}".zip)
  local archive_path="${archive_files[0]}"

  mv "${installer_path}" "${installer_path//x64/x86_64}"
  mv "${pdb_archive_path}" "${pdb_archive_path//-pdbs/-debug-symbols}"

  installer_path="$(cygpath --windows "${installer_path//x64/x86_64}")"
  pdb_archive_path="$(cygpath --windows "${pdb_archive_path//-pdbs/-debug-symbols}")"
  archive_path="$(cygpath --windows "${archive_path}")"

  {
    echo "installer-path=${installer_path}"
    echo "pdb-zip-path=${pdb_archive_path}"
    echo "build-zip-path=${archive_path}"
  } >> "${GITHUB_OUTPUT}"
}

create-installer
