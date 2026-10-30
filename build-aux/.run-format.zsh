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

## Enable for script debugging
# setopt XTRACE

check_linter() {
  local -i found=0
  local linter=${1}
  local min_version
  local version_number

  case ${linter} {
    clang-format)
      if (( ${+commands[clang-format-22]} )) {
        linter='clang-format-22'
        found=1
      } elif (( ${+commands[clang-format]} )) {
        linter='clang-format'
        found=1
      }

      if (( found )) {
        min_version='22.1.3'

        local -a clang_format_version
        read -r -A clang_format_version <<< "$(${linter} --version 2>/dev/null || true)"
        version_number=${clang_format_version[-1]}
      }
      ;;
    swift-format)
      if (( ${+commands[swift-format]} )) {
        linter='swift-format'
        found=1
      }

      if (( found )) {
        min_version='602.0.0'
        version_number="$(swift-format --version 2>/dev/null || true)"
      }
      ;;
    gersemi)
      if (( ${+commands[gersemi]} )) {
        linter='gersemi'
        found=1
      }

      if (( found )) {
        min_version='0.27.0'
        local -a gersemi_version
        read -r -A gersemi_version <<< "$(gersemi --version 2>/dev/null || true)"
        version_number=${gersemi_version[2]}
      }
      ;;
    zizmor)
      if (( ${+commands[zizmor]} )) {
        linter='zizmor'
        found=1
      }

      if (( found )) {
        min_version='1.25.0'
        local -a zizmor_version
        read -r -A zizmor_version <<< "$(zizmor --version 2>/dev/null || true)"
        version_number=${zizmor_version[2]}
      }
      ;;
    xmllint)
      if (( ${+commands[xmllint]} )) {
        linter='xmllint'
        found=1
      }

      if (( found )) {
        min_version='20900.0.0'
        local -a xmllint_version
        read -r -A xmllint_version <<< "$(xmllint --version 2>&1 || true)"
        version_number="${xmllint_version[5]}.0.0"
      }
      ;;
    *)
      print -u2 -PR "%F{1}  ✖  %f Unsupported linter specified."
      return 1
      ;;
  }

  if (( ! found )) {
    print -u2 -PR "%F{1}  ✖  %f Unable to find %B'${linter}'%b on system."
    return 1
  }

  if ! is-at-least ${min_version} ${version_number}; then
    print -u2 -PR "%F{1}  ✖  %f ${linter} ${version_number} found (Required: %B${min_version}%b)."
    return 1
  fi
}

generate_file_list() {
  local linter=${1}
  local -a found_files

  if (( ! #source_files )) {
    case ${linter} {
      clang-format)
        found_files=((libobs|libobs-*|frontend|plugins|deps|shared|test)/**/*.(c|cpp|h|hpp|m|mm)(.N))
        found_files=(${found_files:#*/(decklink/*/decklink-sdk|obs-websocket|obs-browser|libdshowcapture)/*})
        ;;
      swift-format)
        found_files=((libobs|libobs-*|frontend|plugins)/**/*.swift(.N))
        ;;
      gersemi)
        found_files=(CMakeLists.txt (libobs|libobs-*|frontend|plugins|deps|shared|cmake|test)/**/(CMakeLists.txt|*.cmake)(.N))
        found_files=(${found_files:#*/(jansson|decklink/*/decklink-sdk|obs-websocket|obs-browser|libdshowcapture)/*})
        ;;
      zizmor)
        found_files=(.github/(workflows|actions)/**/*.(yaml|yml))
        ;;
      xmllint)
        found_files=(frontend/forms/**/*ui)
        ;;
      *) return ;;
    }

    typeset -ga source_files=(${found_files})
  } else {
    typeset -ga source_files=(${source_files//${project_root}\/})
  }
}

invoke_formatter() {
  local formatter=${1}
  shift
  local -a source_files
  read -r -A source_files <<< "${@}"
  local -a format_arguments

  generate_file_list ${formatter}

  case ${formatter} {
    clang-format)
      format_arguments=(--style=file --fallback-style=none -i)
      if (( verbose_output )) {
        format_arguments+=(--verbose)
      }
      ;;
    swift-format)
      format_arguments=(format --parallel --color-diagnostics -i)
      ;;
    gersemi)
      format_arguments=(--no-cache -i)
      ;;
    *)
      return 1
  }

  ${formatter} ${format_arguments} ${source_files}
}

invoke_linter() {
  local linter=${1}
  shift
  local -a source_files
  read -r -A source_files <<< "${@}"

  local regexp
  local glob_expression
  local -a indices
  local -a lint_arguments

  generate_file_list ${linter}

  case ${linter} {
    clang-format)
      regexp='^([^:]+):([0-9]+):[0-9]+:[[:space:]](.+):[[:space:]](.+)\[-W(.+)\]$'
      indices=(1 2 3 5 4)
      lint_arguments=(--style=file --fallback-style=none -Werror --dry-run)
      if (( verbose_output )) {
        lint_arguments+=(--verbose)
      }
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
      lint_arguments=(--schema ${project_root}/frontend/forms/XML-Schema-Qt5.15.xsd --noout)
      ;;
    *)
      return 1
      ;;
  }

  local -i num_failures=0

  if (( #source_files )) {
    local file_path
    local line_number
    local error_level
    local error_title
    local error_message

    local line
    local ordered_output
    local -a match
    local mbegin
    local mend
    local MATCH
    local MBEGIN
    local MEND

    while read -r line; do
      if [[ ${linter} == 'zizmor' ]] && (( github_style )) {
        print -- ${line}
        num_failures+=1
        continue
      }

      if [[ ${line} =~ ${regexp} ]] {
        ordered_output="${(j:|:)indices//(#m)<0-9>##/${match[${MATCH}]}}"
        { IFS='|' read -r file_path line_number error_level error_title error_message } <<< "${ordered_output}"

        if (( github_style )) {
          print "::${error_level} file=${file_path},line=${line_number},title=${error_title}::${file_path:t}:${line_number}: ${error_message}"
        } else {
          print -PR "  %F{1}✖%f  ${file_path}:${line_number} - ${error_title}: ${error_message}"
        }

        num_failures+=1
      } else {
        print -- "${line}"
      }
    done < <(${linter} ${lint_arguments} ${source_files} 2>&1 || true)
  }

  if (( num_failures )) {
    return 1
  }
}

main() {
  autoload -Uz is-at-least && if ! is-at-least 5.8; then
    print -u2 -PR "%F{1}  ✖  %f ${ZSH_ARGZERO} requires Zsh %B5.8%b or later (detected version: %B${ZSH_VERSION}%b)."
    exit 1
  fi

  if (( ! ${+SCRIPT_HOME} )) typeset -g SCRIPT_HOME="${ZSH_ARGZERO:A:h}"
  local project_root=${SCRIPT_HOME:A:h}

  local -i verbose_output=0
  local -i lint_only=0
  local -i github_style=0

  local -a args
  while (( # )) {
    case ${1} {
      -c|--check) lint_only=1; shift ;;
      -v|--verbose) verbose_output=1; shift ;;
      -gh|--github) github_style=1; shift ;;
      -l|--linter)
        if (( ! ${+LINTER_NAME} )) && [[ -n ${2} ]] {
          typeset -g LINTER_NAME="${2}"
          shift 2
        }
        ;;
      *)
        args+=(${1})
        shift
        ;;
    }
  }

  check_linter ${LINTER_NAME}

  if (( lint_only )) {
    invoke_linter ${LINTER_NAME} ${args}
  } else {
    invoke_formatter "${LINTER_NAME}" ${args}
  }
}

main ${@}
