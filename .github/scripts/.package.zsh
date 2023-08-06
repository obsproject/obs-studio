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

if (( ! ${+CI} )) {
  print -u2 -PR "%F{1}    ✖︎ ${ZSH_ARGZERO:t:r} requires CI environment%f"
  exit 1
}

autoload -Uz is-at-least && if ! is-at-least 5.2; then
  print -u2 -PR "%F{1}${funcstack[1]##*/}:%f Running on Zsh version %B${ZSH_VERSION}%b, but Zsh %B5.2%b is the minimum supported version. Upgrade Zsh to fix this issue."
  exit 1
fi

TRAPZERR() {
  print -u2 -PR "::error::%F{1}    ✖︎ script execution error%f"
  print -PR -e "
  Callstack:
  ${(j:\n     :)funcfiletrace}
  "

  exit 2
}

package() {
  if (( ! ${+SCRIPT_HOME} )) typeset -g SCRIPT_HOME=${ZSH_ARGZERO:A:h}
  local host_os=${${(s:-:)ZSH_ARGZERO:t:r}[2]}
  local project_root=${SCRIPT_HOME:A:h:h}
  local buildspec_file=${project_root}/buildspec.json

  fpath=(${SCRIPT_HOME}/utils.zsh ${fpath})
  autoload -Uz log_error log_output log_group check_${host_os}

  local -i debug=0

  local target
  local -r -a _valid_targets=(
    macos-x86_64
    macos-arm64
    linux-x86_64
  )

  local config='RelWithDebInfo'
  local -r -a _valid_configs=(Debug RelWithDebInfo Release MinSizeRel)

  local -i codesign=0
  local -i notarize=0
  local -i package=0
  local -i skip_deps=0

  local -a args
  while (( # )) {
    case ${1} {
      -t|--target|-c|--config)
        if (( # == 1 )) || [[ ${2:0:1} == '-' ]] {
          log_error "Missing value for option %B${1}%b"
          exit 2
        }
        ;;
    }
    case ${1} {
      --) shift; args+=($@); break ;;
      -t|--target)
        if (( ! ${_valid_targets[(Ie)${2}]} )) {
          log_error "Invalid value %B${2}%b for option %B${1}%b"
          exit 2
        }
        target=${2}
        shift 2
        ;;
      -c|--config)
        if (( ! ${_valid_configs[(Ie)${2}]} )) {
          log_error "Invalid value %B${2}%b for option %B${1}%b"
          exit 2
        }
        config=${2}
        shift 2
        ;;
      -s|--codesign) codesign=1; shift ;;
      -n|--notarize) notarize=1; shift ;;
      -p|--package) typeset -g package=1; shift ;;
      --debug) debug=0; shift ;;
      *) log_error "Unknown option: %B${1}%b"; log_output ${_usage}; exit 2 ;;
    }
  }

  : "${target:="${host_os}-${CPUTYPE}"}"

  set -- ${(@)args}

  check_${host_os}

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

      typeset -gx CODESIGN_IDENT="${CODESIGN_IDENT:--}"
      typeset -gx CODESIGN_TEAM="$(print "${CODESIGN_IDENT}" | /usr/bin/sed -En 's/.+\((.+)\)/\1/p')"

      codesign --sign "${CODESIGN_IDENT}" ${output_name}.dmg

      if (( codesign && notarize )) {
        if ! [[ ${CODESIGN_IDENT} != '-' && ${CODESIGN_TEAM} && ${CODESIGN_IDENT_USER} && ${CODESIGN_IDENT_PASS} ]] {
          log_error "Notarization requires Apple ID and application password."
          return 2
        }

        xcrun notarytool store-credentials 'OBS-Codesign-Password' --apple-id "${CODESIGN_IDENT_USER}" --team-id "${CODESIGN_TEAM}" --password "${CODESIGN_IDENT_PASS}"
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
      XZ_OPT=-T0 tar -cvJf ${output_name}.tar.xz OBS.app
      popd
    }

    if [[ ${config} == Release ]] {
      log_group "Archiving debug symbols..."
      mkdir -p build_macos/dSYMs
      pushd build_macos/dSYMs
      rm -rf -- *.dSYM(N)
      cp -pR ${PWD:h}/**/*.dSYM .
      XZ_OPT=-T0 tar -cvJf ${output_name}-dSYMs.tar.xz -- *
      mv ${output_name}-dSYMs.tar.xz ${PWD:h}
      popd
    }

    log_group

  } elif [[ ${host_os} == linux ]] {
    local cmake_bin='/usr/bin/cmake'
    local -a cmake_args=()
    if (( debug )) cmake_args+=(--verbose)

    if (( package )) {
      log_group "Packaging obs-studio..."
      pushd ${project_root}
      ${cmake_bin} --build build_${target##*-} --config ${config} -t package ${cmake_args}
      output_name="${output_name}-${target##*-}-linux-gnu"

      pushd ${project_root}/build_${target##*-}
      local -a files=(obs-studio-*-Linux*.(ddeb|deb))
      for file (${files}) {
        mv ${file} ${file//obs-studio-*-Linux/${output_name}}
      }
      popd
      popd
    } else {
      log_group "Archiving obs-studio..."
      output_name="${output_name}-${target##*-}-linux-gnu"

      pushd ${project_root}/build_${target##*-}/install/${config}
      XZ_OPT=-T0 tar -cvJf ${project_root}/build_${target##*-}/${output_name}.tar.xz (bin|lib|share)
      popd
    }
    log_group
  }
}

package ${@}
