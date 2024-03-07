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
  print -u2 -PR "%F{1}    ✖︎ ${ZSH_ARGZERO:t:r} requires CI environment.%f"
  exit 1
}

autoload -Uz is-at-least && if ! is-at-least 5.2; then
  print -u2 -PR "%F{1}${funcstack[1]##*/}:%f Running on Zsh version %B${ZSH_VERSION}%b, but Zsh %B5.2%b is the minimum supported version. Upgrade Zsh to fix this issue."
  exit 1
fi

TRAPZERR() {
  print -u2 -PR "::error::%F{1}    ✖︎ script execution error.%f"
  print -PR -e "
  Callstack:
  ${(j:\n     :)funcfiletrace}
  "

  exit 2
}

build() {
  if (( ! ${+SCRIPT_HOME} )) typeset -g SCRIPT_HOME=${ZSH_ARGZERO:A:h}
  local host_os=${${(s:-:)ZSH_ARGZERO:t:r}[2]}
  local project_root=${SCRIPT_HOME:A:h:h}
  local buildspec_file=${project_root}/buildspec.json

  fpath=(${SCRIPT_HOME}/utils.zsh ${fpath})
  autoload -Uz log_group log_error log_output check_${host_os} setup_ccache

  if [[ ! -r ${buildspec_file} ]] {
    log_error 'Missing buildspec.json in project checkout.'
    return 2
  }

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

  local -a args
  while (( # )) {
    case ${1} {
      -t|--target|--generator|-c|--config)
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
      --debug) debug=1; shift ;;
      *) log_error "Unknown option: %B${1}%b"; log_output ${_usage}; exit 2 ;;
    }
  }

  : "${target:="${host_os}-${CPUTYPE}"}"

  set -- ${(@)args}

  check_${host_os}
  setup_ccache

  if [[ ${host_os} == linux ]] {
    autoload -Uz setup_linux && setup_linux
  }

  local product_name
  read -r product_name <<< "$(jq -r '.name' ${buildspec_file})"

  pushd ${project_root}

  local -a cmake_args=()
  local -a cmake_build_args=(--build)
  local -a cmake_install_args=(--install)

  if (( debug )) cmake_args+=(--debug-output)

  case ${target} {
    macos-*)
      cmake_args+=(--preset 'macos-ci' -DCMAKE_OSX_ARCHITECTURES:STRING=${target##*-})

      typeset -gx NSUnbufferedIO=YES

      typeset -gx CODESIGN_IDENT="${CODESIGN_IDENT:--}"
      if (( codesign )) && [[ -z ${CODESIGN_TEAM} ]] {
        typeset -gx CODESIGN_TEAM="$(print "${CODESIGN_IDENT}" | /usr/bin/sed -En 's/.+\((.+)\)/\1/p')"
      }

      log_group "Configuring ${product_name}..."
      cmake -S ${project_root} ${cmake_args}

      log_group "Building ${product_name}..."
      run_xcodebuild() {
        if (( debug )) {
          xcodebuild ${@}
        } else {
          if [[ ${GITHUB_EVENT_NAME} == push ]] {
            xcodebuild ${@} 2>&1 | xcbeautify --renderer terminal
          } else {
            xcodebuild ${@} 2>&1 | xcbeautify --renderer github-actions
          }
        }
      }

      local -a build_args=(
        ONLY_ACTIVE_ARCH=NO
        -project obs-studio.xcodeproj
        -target obs-studio
        -destination "generic/platform=macOS,name=Any Mac"
        -configuration ${config}
        -parallelizeTargets
        -hideShellScriptEnvironment
        build
      )

      local -a archive_args=(
        ONLY_ACTIVE_ARCH=NO
        -project obs-studio.xcodeproj
        -scheme obs-studio
        -destination "generic/platform=macOS,name=Any Mac"
        -archivePath obs-studio.xcarchive
        -parallelizeTargets
        -hideShellScriptEnvironment
        archive
      )

      local -a export_args=(
        -exportArchive
        -archivePath obs-studio.xcarchive
        -exportOptionsPlist exportOptions.plist
        -exportPath ${project_root}/build_macos
      )

      pushd build_macos
      if [[ ${GITHUB_EVENT_NAME} == push && ${GITHUB_REF_NAME} =~ [0-9]+.[0-9]+.[0-9]+(-(rc|beta).+)? ]] {
        run_xcodebuild ${archive_args}
        run_xcodebuild ${export_args}
      } else {
        run_xcodebuild ${build_args}

        rm -rf OBS.app
        mkdir OBS.app
        ditto UI/${config}/OBS.app OBS.app
      }
      popd
      ;;
    linux-*)
      local cmake_bin='/usr/bin/cmake'
      cmake_args+=(
        -S ${PWD} -B build_${target##*-}
        -G Ninja
        -DCMAKE_BUILD_TYPE:STRING=${config}
        -DCEF_ROOT_DIR:PATH="${project_root}/.deps/cef_binary_${CEF_VERSION}_${target//-/_}"
        -DENABLE_AJA:BOOL=OFF
        -DENABLE_WEBRTC:BOOL=OFF
      )
      if (( ! UBUNTU_2210_OR_LATER )) cmake_args+=(-DENABLE_NEW_MPEGTS_OUTPUT:BOOL=OFF)

      cmake_build_args+=(build_${target##*-} --config ${config} --parallel)
      cmake_install_args+=(build_${target##*-} --prefix ${project_root}/build_${target##*-}/install/${config})

      log_group "Configuring ${product_name}..."
      ${cmake_bin} -S ${project_root} ${cmake_args}

      log_group "Building ${product_name}..."
      if (( debug )) cmake_build_args+=(--verbose)
      ${cmake_bin} ${cmake_build_args}

      log_group "Installing ${product_name}..."
      if (( debug )) cmake_install_args+=(--verbose)
      ${cmake_bin} ${cmake_install_args}
      ;;
  }
  popd
  log_group
}

build ${@}
