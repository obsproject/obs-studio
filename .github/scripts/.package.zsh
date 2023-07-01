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
#setopt WARN_CREATE_GLOBAL
#setopt WARN_NESTED_VAR
#setopt XTRACE

autoload -Uz is-at-least && if ! is-at-least 5.2; then
  print -u2 -PR "%F{1}${funcstack[1]##*/}:%f Running on Zsh version %B${ZSH_VERSION}%b, but Zsh %B5.2%b is the minimum supported version. Upgrade Zsh to fix this issue."
  exit 1
fi

TRAPEXIT() {
  local return_value=$?

  if (( ${+CI} )) {
    unset NSUnbufferedIO
  }

  return ${return_value}
}

TRAPZERR() {
  if (( ${_loglevel:-3} > 2 )) {
    print -u2 -PR "${CI:+::error::}%F{1}    ✖︎ script execution error%f"
    print -PR -e "
    Callstack:
    ${(j:\n     :)funcfiletrace}
    "
  }

  exit 2
}

package() {
  if (( ! ${+SCRIPT_HOME} )) typeset -g SCRIPT_HOME=${ZSH_ARGZERO:A:h}
  local host_os=${${(s:-:)ZSH_ARGZERO:t:r}[2]}
  local project_root=${SCRIPT_HOME:A:h:h}
  local buildspec_file=${project_root}/buildspec.json

  fpath=(${SCRIPT_HOME}/utils.zsh ${fpath})
  autoload -Uz set_loglevel log_info log_error log_output check_${host_os}

  local -i verbosity=1
  local -r _version='1.0.0'
  local -r -a _valid_targets=(
    macos-x86_64
    macos-arm64
    linux-x86_64
  )
  local target
  local config='RelWithDebInfo'
  local -r -a _valid_configs=(Debug RelWithDebInfo Release MinSizeRel)
  local -i codesign=0
  local -i notarize=0
  local -i package=0
  local -i skip_deps=0

  if [[ ${host_os} == macos ]] {
    local -r _usage_host="
%F{yellow} Additional options for macOS builds%f
 -----------------------------------------------------------------------------
  %B-s | --codesign%b                   Enable codesigning (macOS only)
  %B-n | --notarize%b                   Enable notarization (macOS only)"
  }

  local -r _usage="
Usage: %B${functrace[1]%:*}%b <option> [<options>]

%BOptions%b:

%F{yellow} Package configuration options%f
 -----------------------------------------------------------------------------
  %B-t | --target%b                     Specify target - default: %B%F{green}${host_os}-${CPUTYPE}%f%b
  %B-c | --config%b                     Build configuration
  %B-p | --package%b                    Create package installer (macOS only)
  %B--skip-deps%b                       Skip checking for dependencies

%F{yellow} Output options%f
 -----------------------------------------------------------------------------
  %B-q | --quiet%b                      Quiet (error output only)
  %B-v | --verbose%b                    Verbose (more detailed output)
  %B--debug%b                           Debug (very detailed and added output)

%F{yellow} General options%f
 -----------------------------------------------------------------------------
  %B-h | --help%b                       Print this usage help
  %B-V | --version%b                    Print script version information
${_usage_host:-}"

  local -a args
  while (( # )) {
    case ${1} {
      -t|--target|-c|--config)
        if (( # == 1 )) || [[ ${2:0:1} == '-' ]] {
          log_error "Missing value for option %B${1}%b"
          log_output ${_usage}
          exit 2
        }
        ;;
    }
    case ${1} {
      --)
        shift
        args+=($@)
        break
        ;;
      -t|--target)
        if (( ! ${_valid_targets[(Ie)${2}]} )) {
          log_error "Invalid value %B${2}%b for option %B${1}%b"
          log_output ${_usage}
          exit 2
        }
        target=${2}
        shift 2
        ;;
      -c|--config)
        if (( ! ${_valid_configs[(Ie)${2}]} )) {
          log_error "Invalid value %B${2}%b for option %B${1}%b"
          log_output ${_usage}
          exit 2
        }
        config=${2}
        shift 2
        ;;
      -s|--codesign) CODESIGN=1; shift ;;
      -n|--notarize) NOTARIZE=1; shift ;;
      -p|--package) typeset -g package=1; shift ;;
      --skip-deps) typeset -g skip_deps=1; shift ;;
      -q|--quiet) (( verbosity -= 1 )) || true; shift ;;
      -v|--verbose) (( verbosity += 1 )); shift ;;
      -h|--help) log_output ${_usage}; exit 0 ;;
      -V|--version) print -Pr "${_version}"; exit 0 ;;
      --debug) verbosity=3; shift ;;
      *) log_error "Unknown option: %B${1}%b"; log_output ${_usage}; exit 2 ;;
    }
  }

  : "${target:="${host_os}-${CPUTYPE}"}"

  set -- ${(@)args}
  set_loglevel ${verbosity}

  if (( ! skip_deps )) {
    check_${host_os}
  }

  local product_name
  read -r product_name <<< \
    "$(jq -r '.name' ${buildspec_file})"

  local commit_version='0.0.0'
  local commit_distance='0'
  local commit_hash

  if [[ -d ${project_root}/.git ]] {
    local git_description="$(git describe --tags --long)"
    commit_version="${${git_description%-*}%-*}"
    commit_hash="${git_description##*-g}"
    commit_distance="${${git_description%-*}##*-}"
  }


  local output_name
  if (( commit_distance > 0 )) {
    output_name="obs-studio-${commit_version}-${commit_hash}"
  } else {
    output_name="obs-studio-${commit_version}"
  }

  if [[ ${host_os} == macos ]] {
    autoload -Uz read_codesign read_codesign_pass log_warning log_group

    if [[ ! -d build_macos/OBS.app ]] {
      log_error 'No application bundle found. Run the build script to create a valid application bundle.'
      return 0
    }

    local -A arch_names=(x86_64 Intel arm64 Apple)
    output_name="${output_name}-macos-${(L)arch_names[${target##*-}]}"

    local volume_name
    if (( commit_distance > 0 )) {
      volume_name="OBS Studio ${commit_version}-${commit_hash} (${arch_names[${target##*-}]})"
    } else {
      volume_name="OBS Studio ${commit_version} (${arch_names[${target##*-}]})"
    }

    local _tarflags='cJf'
    if (( _loglevel > 1 || ${+CI} )) _tarflags="v${_tarflags}"

    if (( package )) {
      pushd build_macos

      mkdir -p obs-studio/.background
      cp ${project_root}/cmake/macos/resources/background.tiff obs-studio/.background/
      cp ${project_root}/cmake/macos/resources/AppIcon.icns obs-studio/.VolumeIcon.icns
      ln -s /Applications obs-studio/Applications

      mkdir -p obs-studio/OBS.app
      ditto OBS.app obs-studio/OBS.app

      local -i _status=0

      autoload -Uz create_diskimage
      create_diskimage obs-studio ${volume_name} ${output_name} || _status=1

      rm -r obs-studio
      if (( _status )) {
        log_error "Disk image creation failed."
        return 2
      }

      if (( codesign )) { autoload -Uz read_codesign && read_codesign }

      codesign --sign "${CODESIGN_IDENT:--}" ${output_name}.dmg

      if (( codesign && notarize )) {
        autoload -Uz read_codesign_pass && read_codesign_pass

        xcrun notarytool submit "${output_name}".dmg --keychain-profile "OBS-Codesign-Password" --wait

        local -i _status=0

        xcrun stapler staple ${output_name}.dmg || _status=1

        if (( _status )) {
          log_error "Notarization failed. Use 'xcrun notarytool log <submission ID>' to check errors."
          return 2
        }
      }
      popd
    } else {
      log_group "Archiving obs-studio..."
      pushd build_macos
      XZ_OPT=-T0 tar "-${_tarflags}" ${output_name}.tar.xz OBS.app
      popd
    }

    if [[ ${config} == Release ]] {
      log_group "Archiving debug symbols..."
      mkdir -p build_macos/dSYMs
      pushd build_macos/dSYMs
      rm -rf -- *.dSYM(N)
      cp -pR ${PWD:h}/**/*.dSYM .
      XZ_OPT=-T0 tar "-${_tarflags}" ${output_name}-dSYMs.tar.xz -- *
      mv ${output_name}-dSYMs.tar.xz ${PWD:h}
      popd
    }

    log_group
  }
}

package ${@}
