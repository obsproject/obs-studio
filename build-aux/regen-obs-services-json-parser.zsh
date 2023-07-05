#!/usr/bin/env zsh

builtin emulate -L zsh
setopt EXTENDED_GLOB
setopt PUSHD_SILENT
setopt ERR_EXIT
setopt ERR_RETURN
setopt NO_UNSET
setopt PIPE_FAIL
setopt NO_AUTO_PUSHD
setopt NO_PUSHD_IGNORE_DUPS
setopt FUNCTION_ARGZERO

## Enable for script debugging
# setopt WARN_CREATE_GLOBAL
# setopt WARN_NESTED_VAR
# setopt XTRACE

autoload -Uz is-at-least && if ! is-at-least 5.2; then
  print -u2 -PR "%F{1}${funcstack[1]##*/}:%f Running on Zsh version %B${ZSH_VERSION}%b, but Zsh %B5.2%b is the minimum supported version. Upgrade zsh to fix this issue."
  exit 1
fi

invoke_quicktype() {
  local json_parser_file="plugins/obs-services/generated/services-json.hpp"

  if (( ${+commands[quicktype]} )) {
    local quicktype_version=($(quicktype --version))

    if ! is-at-least 23.0.0 ${quicktype_version[3]}; then
      log_error "quicktype is not version 23.x.x (found ${quicktype_version[3]}."
      exit 2
    fi

    if is-at-least 24.0.0 ${quicktype_version[3]}; then
      log_error "quicktype is more recent than version 23.x.x (found ${quicktype_version[3]})."
      exit 2
    fi

  } else {
    log_error "No viable quicktype version found (required 23.x.x)"
    exit 2
  }

  quicktype_args+=(
    ## Main schema and its dependencies
    -s schema build-aux/json-schema/obs-services.json
    -S build-aux/json-schema/service.json
    -S build-aux/json-schema/protocolDefs.json
    -S build-aux/json-schema/codecDefs.json
    ## Language and language options
    -l cpp --no-boost
    --code-format with-struct
    --type-style pascal-case
    --member-style camel-case
    --enumerator-style upper-underscore-case
    # Global include for Nlohmann JSON
    --include-location global-include
    # Namespace
    --namespace OBSServices
    # Main struct type name
    -t ServicesJson
  )

  if (( check_only )) {
    if (( _loglevel > 1 )) log_info "Checking obs-services JSON parser code..."

    if ! quicktype ${quicktype_args} | diff -q "${json_parser_file}" - &> /dev/null; then
      log_error "obs-services JSON parser code does not match JSON schemas."
      if (( fail_on_error )) return 2;
    else
      if (( _loglevel > 1 )) log_status "obs-services JSON parser code matches JSON schemas."
    fi
  } else {
    if ! quicktype ${quicktype_args} -o "${json_parser_file}"; then
      log_error "An error occured while generating obs-services JSON parser code."
      if (( fail_on_error )) return 2;
    fi
  }
}

run_quicktype() {
  if (( ! ${+SCRIPT_HOME} )) typeset -g SCRIPT_HOME=${ZSH_ARGZERO:A:h}

  typeset -g host_os=${${(L)$(uname -s)}//darwin/macos}
  local -i fail_on_error=0
  local -i check_only=0
  local -i verbosity=1
  local -r _version='1.0.0'

  fpath=("${SCRIPT_HOME}/.functions" ${fpath})
  autoload -Uz set_loglevel log_info log_error log_output log_status log_warning

  local -r _usage="
Usage: %B${functrace[1]%:*}%b <option>

%BOptions%b:

%F{yellow} Formatting options%f
 -----------------------------------------------------------------------------
  %B-c | --check%b                      Check only, no actual formatting takes place

%F{yellow} Output options%f
 -----------------------------------------------------------------------------
  %B-v | --verbose%b                    Verbose (more detailed output)
  %B--fail-[never|error]                Fail script never/on formatting change - default: %B%F{green}never%f%b
  %B--debug%b                           Debug (very detailed and added output)

%F{yellow} General options%f
 -----------------------------------------------------------------------------
  %B-h | --help%b                       Print this usage help
  %B-V | --version%b                    Print script version information"

  local -a args
  while (( # )) {
    case ${1} {
      --)
        shift
        args+=($@)
        break
        ;;
      -c|--check) check_only=1; shift ;;
      -v|--verbose) (( verbosity += 1 )); shift ;;
      -h|--help) log_output ${_usage}; exit 0 ;;
      -V|--version) print -Pr "${_version}"; exit 0 ;;
      --debug) verbosity=3; shift ;;
      --fail-never)
        fail_on_error=0
        shift
        ;;
      --fail-error)
        fail_on_error=1
        shift
        ;;
      --fail-fast)
        fail_on_error=2
        shift
        ;;
      *) log_error "Unknown option: %B${1}%b"; log_output ${_usage}; exit 2 ;;
    }
  }

  set -- ${(@)args}
  set_loglevel ${verbosity}

  invoke_quicktype
}

run_quicktype ${@}
