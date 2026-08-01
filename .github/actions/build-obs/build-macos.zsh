#!/usr/bin/env zsh

builtin emulate -L zsh
setopt PUSHD_SILENT
setopt EXTENDED_GLOB
setopt ERR_EXIT
setopt ERR_RETURN
setopt NO_UNSET
setopt PIPE_FAIL
setopt NO_AUTO_PUSHD
setopt NO_PUSHD_IGNORE_DUPS
setopt NO_GLOB_SUBST
setopt WARN_CREATE_GLOBAL
setopt WARN_NESTED_VAR

: ${CI:?}
if (( ${+RUNNER_DEBUG} )) setopt XTRACE

run_xcodebuild() {
  if (( ${+RUNNER_DEBUG} )) || [[ ${ANALYZE:-false} == 'true' ]] {
    env NSUnbufferedIO=YES xcodebuild ${@} 2>&1 | xcbeautify --preserve-unbeautified --renderer terminal
  } else {
    env NSUnbufferedIO=YES xcodebuild ${@} 2>&1 | xcbeautify --renderer github-actions
  }
}

build-macos() {
  local checkout="${PWD}"

  if ! [[ -d ${checkout}/.git && -r ${checkout}/CMakePresets.json ]] {
    print '::error::Action needs to be run from the root directory of an obs-studio checkout.'
    return 1
  }

  if (( ${+RUNNER_DEBUG} )) {
    cmake --version
  }

  typeset -gx CODESIGN_IDENT
  typeset -gx CODESIGN_TEAM

  mkdir -p ${OUTPUT_PATH}

  local build_dir
  {
    local preset_build_dir
    preset_build_dir="$(jq --raw-output '
      .configurePresets[] | select(.name == "macos") | .binaryDir
    ' ${checkout}/CMakePresets.json)"
    build_dir="${preset_build_dir//\$\{sourceDir\}/${OUTPUT_PATH}}"
  }

  if (( ! ${+CODESIGN_TEAM} )) {
    CODESIGN_TEAM="${${CODESIGN_IDENT##*\(}%%\)*}"
    print "::add-mask::${CODESIGN_TEAM}"
  }

  print '::group::Configure obs-studio'
  local -a cmake_args=(
    --preset macos-ci
    -B ${build_dir}
    "-DCMAKE_OSX_ARCHITECTURES:STRING=${BUILD_TARGET}"
  )

  if (( ${+RUNNER_DEBUG} )) {
    cmake_args+=(
      --log-level=DEBUG
      -DCMAKE_XCODE_ATTRIBUTE_COMPILATION_CACHE_ENABLE_DIAGNOSTIC_REMARKS:STRING=YES
    )
  }

  cmake ${cmake_args}
  print '::endgroup::'

  print '::group::Build obs-studio'
  if [[ ! -d ${build_dir} ]] {
    print "::echo::Expected build directory '${build_dir}' not found."
    return 1
  }

  pushd ${build_dir}

  local -a common_args=(
    ONLY_ACTIVE_ARCH=NO -project obs-studio.xcodeproj
    -destination 'generic/platform=macOS,name=Any Mac'
    -parallelizeTargets -hideShellScriptEnvironment
  )

  if [[ ${ANALYZE:-false} == 'true' ]] {
    local -a analyze_args=(
      CLANG_ANALYZER_OUTPUT=sarif
      CLANG_ANALYZER_OUTPUT_DIR=${build_dir}/analytics
      ${common_args}
      -target obs-studio
      -configuration ${BUILD_CONFIG}
      analyze
    )

    run_xcodebuild ${analyze_args}

    print "analyzer-output-path=${build_dir}/analytics" >> ${GITHUB_OUTPUT}
  } else {
    local match
    local mbegin
    local mend
    local version_regex='[0-9]+\.[0-9]+\.[0-9]+(-(rc|beta).+)?'
    if [[ "${GITHUB_EVENT_NAME:-}" == 'push' && "${GITHUB_REF_NAME:-}" =~ ${version_regex} && -n ${CODESIGN_TEAM} ]] {
      common_args+=(-scheme obs-studio -archivePath obs-studio.xcarchive archive)

      local -a export_args=(
        -exportArchive -archivePath obs-studio.xcarchive -exportOptionsPlist exportOptions.plist
        -exportPath ${OUTPUT_PATH}
      )

      run_xcodebuild ${common_args}
      run_xcodebuild ${export_args}
    } else {
      common_args+=(-scheme obs-studio -configuration "${BUILD_CONFIG}" build)

      run_xcodebuild ${common_args}

      local app_bundle="${build_dir}/frontend/${BUILD_CONFIG}/OBS.app"

      if [[ ! -d  ${app_bundle} ]] {
        print "::error::Expected application bundle '${app_bundle}' not found."
        return 1
      }

      mkdir ${OUTPUT_PATH}/OBS.app
      ditto ${app_bundle} ${OUTPUT_PATH}/OBS.app
    }
  }
  popd
  print '::endgroup::'
}

build-macos
