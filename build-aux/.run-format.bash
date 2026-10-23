#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

## Enable for script debugging
# set -x

shopt -s extglob
shopt -s globstar

check_version() {
  local version=()
  local checked_version=()

  { IFS='.' read -r -a version; } <<< "${1}"
  { IFS='.' read -r -a checked_version; } <<< "${2}"

  if (( version[0] >= checked_version[0]
    && version[1] >= checked_version[1]
    && version[2] >= checked_version[2] )); then
      return 0
  else
    return 1
  fi
}

check_linter() {
  local -i found=0
  local linter="${1}"
  local min_version=''
  local version_number=''

  case "${linter}" in
    clang-format)
      if command -v clang-format-22 > /dev/null; then
        linter='clang-format-22'
        found=1
      elif command -v clang-format > /dev/null; then
        linter='clang-format'
        found=1
      fi

      if (( found )); then
        min_version='22.1.3'

        local -a clang_format_version
        read -r -a clang_format_version <<< "$("${linter}" --version 2>/dev/null || true)"
        local -i last_index="$(( ${#clang_format_version[@]} - 1 ))"
        version_number="${clang_format_version[${last_index}]}"
      fi
      ;;
    swift-format)
      if command -v swift-format > /dev/null; then
        linter='swift-format'
        found=1
      fi

      if (( found )); then
        min_version='602.0.0'
        version_number="$(swift-format --version 2>/dev/null || true)"
      fi
      ;;
    gersemi)
      if command -v gersemi > /dev/null; then
        linter='gersemi'
        found=1
      fi

      if (( found )); then
        min_version='0.27.0'
        local -a gersemi_version
        read -r -a gersemi_version <<< "$(gersemi --version 2>/dev/null || true)"
        version_number="${gersemi_version[1]}"
      fi
      ;;
    zizmor)
      if command -v zizmor > /dev/null; then
        linter='zizmor'
        found=1
      fi

      if (( found )); then
        min_version='1.25.0'
        local -a zizmor_version
        read -r -a zizmor_version <<< "$(zizmor --version 2>/dev/null || true)"
        version_number="${zizmor_version[1]}"
      fi
      ;;
    xmllint)
      if command -v xmllint > /dev/null; then
        linter='xmllint'
        found=1
      fi

      if (( found )); then
        min_version='20900.0.0'
        local -a xmllint_version
        read -r -a xmllint_version <<< "$(xmllint --version 2>&1 || true)"
        version_number="${xmllint_version[4]}.0.0"
      fi
      ;;
    *)
      echo "  ${_red}✖${_reset}  Unsupported linter specified."
      return 1
      ;;
  esac

  if (( ! found )); then
    echo "  ${_red}✖${_reset}  Unable to find '${linter}' on system."
    return 1
  fi

  if ! check_version "${version_number}" "${min_version}"; then
    echo "  ${_red}✖${_reset}  ${linter} ${version_number} found (Required: ${min_version})."
    return 1
  fi
}

generate_file_list() {
  local linter="${1}"

  if (( ! ${#source_files[@]} )); then
    case "${linter}" in
    clang-format)
        source_files=(@(libobs|libobs-*|frontend|plugins|deps|shared|test)/**/*.@(c|cpp|h|hpp|m|mm))
        read -r -a source_files <<< "${source_files[@]//*\/@(decklink\/*\/decklink-sdk|obs-websocket|obs-browser|libdshowcapture)\/*/}"
        ;;
      swift-format)
        source_files=(@(libobs|libobs-*|frontend|plugins)/**/*.swift)
        ;;
      gersemi)
        source_files=(CMakeLists.txt @(libobs|libobs-*|frontend|plugins|deps|shared|cmake|test)/**/@(CMakeLists.txt|*.cmake))
        read -r -a source_files <<< "${source_files[@]//*\/@(jansson|decklink\/*\/decklink-sdk|obs-websocket|obs-browser|libdshowcapture)\/*/}"
        ;;
      zizmor)
        source_files=(.github/@(workflows|actions)/**/*.@(yaml|yml))
        ;;
      xmllint)
        source_files=(frontend/forms/**/*.ui)
        ;;
      *) ;;
    esac
  else
    source_files=("${source_files[@]//${project_root}\/}")
  fi
}

invoke_formatter() {
  local formatter="${1}"
  shift
  local -a source_files
  read -r -a source_files <<< "${@}"
  local -a format_arguments

  generate_file_list "${formatter}"

  case "${formatter}" in
    clang-format)
      format_arguments=(--style=file --fallback-style=none -i)
      if (( verbose_output )); then
        format_arguments+=('--verbose')
      fi
      ;;
    swift-format)
      format_arguments=(format --parallel --color-diagnostics -i)
      ;;
    gersemi)
      format_arguments=(--no-cache -i)
      ;;
    *)
      return 1
      ;;
  esac

  "${formatter}" "${format_arguments[@]}" "${source_files[@]}"
}

invoke_linter() {
  local linter="${1}"
  shift
  local -a source_files
  read -r -a source_files <<< "${@}"

  local regexp
  local -a indices
  local -a lint_arguments

  generate_file_list "${linter}"

  case "${linter}" in
    clang-format)
      regexp='^([^:]+):([0-9]+):[0-9]+:[[:space:]](.+):[[:space:]](.+)\[-W(.+)\]$'
      indices=(1 2 3 5 4)
      lint_arguments=(--style=file --fallback-style=none -Werror --dry-run)
      if (( verbose_output )); then
        lint_arguments+=(--verbose)
      fi
      ;;
    swift-format)
      regexp='^([^:]+):([0-9]+):[0-9]+:[[:space:]](.+):[[:space:]]\[(.+)\][[:space:]](.+)$'
      indices=(1 2 3 4 5)
      lint_arguments=(lint)
     ;;
    gersemi)
      regexp="^${project_root}/([^[:space:]]+)[[:space:]](.+)"
      indices=(1 'Entire File' 'error' 'gersemi' 2)
      lint_arguments=(--check --no-cache --warnings-as-errors)
      ;;
    zizmor)
      regexp='^::(.+)[[:space:]]file=(.+),line=([0-9]+),title=(.+)::.+:[0-9]+:[[:space:]](.+)$'
      indices=(2 3 1 4 5)
      lint_arguments=(--offline --persona=auditor --format=github --no-progress --quiet)
      ;;
    xmllint)
      regexp='^([^:]+):([0-9]+):[[:space:]]+.+:[[:space:]](.+):[[:space:]](.+)$'
      indices=(1 2 error 3 4)
      lint_arguments=(--schema "${project_root}/frontend/forms/XML-Schema-Qt5.15.xsd" --noout)
      ;;
    *)
      return 1
      ;;
  esac

  local -i num_failures=0

  if (( ${#source_files[@]} )); then
    local line
    local file_path
    local file_name
    local line_number
    local error_level
    local error_title
    local error_message
    local -a BASH_REMATCH
    while read -r line; do
      local ordered_output=''
      if [[ "${linter}" == 'zizmor' ]] && (( github_style )); then
        echo "${line}"
        num_failures+=1
        continue
      fi

      if [[ "${line}" =~ ${regexp} ]]; then
        for index in "${indices[@]}"; do
          if [[ "${index}" = [[:digit:]] ]]; then
            ordered_output+="${BASH_REMATCH[${index}]}|"
          else
            ordered_output+="${index}|"
          fi
        done

        { IFS="|" read -r file_path line_number error_level error_title error_message; } <<< "${ordered_output}"

        if (( github_style )); then
          file_name="$(basename "${file_path}")"
          echo "::${error_level} file=${file_path},line=${line_number},title=${error_title}::${file_name}:${line_number}: ${error_message}"
        else
          echo "  ${_red}✖${_reset}  ${file_path}:${line_number} - ${error_title}: ${error_message}"
        fi

        num_failures+=1
      else
        echo "${line}"
      fi
    done < <("${linter}" "${lint_arguments[@]}" "${source_files[@]}" 2>&1 || true)
  fi

  if (( num_failures )); then
    return 1
  fi
}

main() {
  local _red=''
  local _reset=''

  if [[ -z "${CI:-}" ]]; then
    _red="$(tput setaf 1)"
    _reset="$(tput sgr0)"
  fi

  if (( BASH_VERSINFO[0] < 4 )); then
    echo "  ${_red}✖${_reset}  ${0} requires Bash 4.0 or later (detected version: ${BASH_VERSION})."
    exit 1
  fi

  if [[ -z "${SCRIPT_HOME:-}" ]]; then
    local script_realpath
    local script_dirname

    script_realpath="$(realpath "${0}")"
    script_dirname="$(dirname "${script_realpath}")"

    typeset -g SCRIPT_HOME="${script_dirname}"
  fi

  local project_root
  project_root="$(dirname "${SCRIPT_HOME}")"

  local -i verbose_output=0
  local -i lint_only=0
  local -i github_style=0

  local -a args
  while (( ${#} )); do
    case "${1}" in
      -c|--check) lint_only=1; shift ;;
      -v|--verbose) verbose_output=1; shift ;;
      -gh|--github) github_style=1; shift ;;
      -l|--linter)
        if [[ -z "${LINTER_NAME:-}" && -n "${2}" ]]; then
          typeset -g LINTER_NAME="${2}"
          shift 2
        fi
        ;;
      *)
        args+=("${1}")
        shift
        ;;
    esac
  done

  check_linter "${LINTER_NAME}"

  if (( lint_only )); then
    invoke_linter "${LINTER_NAME}" "${args[@]}"
  else
    invoke_formatter "${LINTER_NAME}" "${args[@]}"
  fi
}

main "${@}"
