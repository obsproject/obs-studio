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
setopt WARN_CREATE_GLOBAL
setopt WARN_NESTED_VAR

: ${CI:?}
if (( ${+RUNNER_DEBUG} )) setopt XTRACE

create-disk-image() {
  print '::group::Create Disk Image'
  local background_file="${checkout}/cmake/macos/resources/background.tiff"
  local appicon_file="${checkout}/cmake/macos/resources/AppIcon.icns"
  local disk_source="${OUTPUT_PATH}/obs-studio"

  mkdir -p "${disk_source}/.background"
  cp ${background_file} ${disk_source}/.background/
  cp ${appicon_file} ${disk_source}/.VolumeIcon.icns
  ln -s /Applications ${disk_source}/Applications

  mkdir -p ${disk_source}/OBS.app
  ditto ${OUTPUT_PATH}/OBS.app ${disk_source}/OBS.app

  hdiutil create \
    -volname ${volume_name} \
    -srcfolder ${disk_source} \
    -ov \
    -fs APFS \
    -format UDRW \
    ${OUTPUT_PATH}/temp.dmg

  local mount_point="/Volumes/${artifact_name}"

  hdiutil attach \
    -noverify \
    -readwrite \
    -mountpoint ${mount_point} \
    ${OUTPUT_PATH}/temp.dmg

  sleep 2
  SetFile -c icnC ${mount_point}/.VolumeIcon.icns
  osascript ${build_dir}/package.applescript ${artifact_name}
  chmod -Rf go-w ${mount_point}
  SetFile -a C ${mount_point}
  rm -rf -- ${mount_point}/.fseventsd(N)

  hdiutil detach ${mount_point}

  hdiutil convert \
    -format ULMO \
    -ov \
    -o ${disk_image} \
    ${OUTPUT_PATH}/temp.dmg

  print '::endgroup::'
}

create-bundle-archive() {
  print '::group::Create Bundle Archive'
  local bundle_dir="${OUTPUT_PATH}/OBS.app"
  local bundle_archive="${OUTPUT_PATH}/${artifact_name}.tar.xz"

  pushd ${bundle_dir:h}
  XZ_OPT='-6 --threads=0' tar --create --verbose --xz --file ${bundle_archive} ${bundle_dir:t}
  popd
  print '::endgroup::'
}

create-dsym-archive() {
  print '::group::Create Debug Symbols Archive'
  local dsym_dir="${OUTPUT_PATH}/dSYMs"
  local dsym_archive="${OUTPUT_PATH}/${artifact_name}-debug-symbols.tar.xz"

  mkdir -p ${dsym_dir}
  pushd ${dsym_dir}
  cp -pR ${build_dir}/**/*.dSYM ${dsym_dir}
  XZ_OPT='-6 --threads=0' tar --create --verbose --xz --file ${dsym_archive} -- *
  popd
  print '::endgroup::'
}

create-developer-archive() {
  print '::group::Create Framework and Libraries For Plugin Development'
  xattr -r -w com.apple.xcode.CreatedByBuildSystem true ${build_dir} || true
  cmake --build ${build_dir} --clean-first --config Release --target obs-frontend-api | xcbeautify

  local install_location="${OUTPUT_PATH}/libobs_release"
  cmake --install ${build_dir} --component Development --config Release --prefix ${install_location}

  local developer_archive="${OUTPUT_PATH}/${artifact_name}-plugin-dev.tar.xz"

  pushd ${install_location}
  XZ_OPT='-6 --threads=0' tar --create --verbose --xz --file ${developer_archive} -- (Frameworks|include|lib)
  popd
  print '::endgroup::'
}

package-macos() {
  local checkout="${PWD}"

  if ! [[ -d ${checkout}/.git && -r ${checkout}/CMakePresets.json ]] {
    print '::error::Action needs to be run from an obs-studio checkout root directory'
    return 1
  }

  local build_dir
  function {
    local preset_build_dir
    preset_build_dir="$(jq --raw-output '
      .configurePresets[] | select(.name == "macos") | .binaryDir
    ' ${checkout}/CMakePresets.json)"
    typeset -g build_dir="${preset_build_dir//\$\{sourceDir\}/${OUTPUT_PATH}}"
  }

  if [[ ! -d ${OUTPUT_PATH}/OBS.app ]] {
    print '::error::No OBS application bundle found.'
    return 1
  }

  local -A commit_info
  function {
    local git_description
    git_description="$(git describe --tags --long)"

    local version_regex='^([0-9]+\.[0-9]+\.[0-9]+(-(rc|beta).+)?)-([0-9]+)-g([[:alnum:]]+)$'

    local match
    local mbegin
    local mend
    if [[ ${git_description} =~ ${version_regex} ]] {
      typeset -g commit_info=(
        [version]=${match[1]}
        [hash]=${match[-1]}
        [distance]=${match[-2]}
      )
    } else {
      print '::error::Unable to detect version from git commit.'
      return 1
    }
  }

  local artifact_name="${OUTPUT_NAME:-"obs-studio-macos-${BUILD_TARGET}-${commit_info[hash]}"}"

  local -A arch_names=(
    [x86_64]=Intel
    [arm64]=Apple
  )
  local volume_name
  if (( ${commit_info[distance]} > 0 )) {
    volume_name="OBS Studio ${commit_info[version]}-${commit_info[hash]} (${arch_names[${BUILD_TARGET}]})"
  } else {
    volume_name="OBS Studio ${commit_info[version]} (${arch_names[${BUILD_TARGET}]})"
  }
  print '::endgroup::'

  if [[ "${BUILD_PACKAGE:-false}" == 'true' ]] {
    local disk_image="${OUTPUT_PATH}/${artifact_name}-unsigned.dmg"

    create-disk-image
  } else {
    create-bundle-archive
  }

  if [[ "${BUILD_CONFIG}" == 'Release' ]] {
    create-dsym-archive
    create-developer-archive
  }
}

package-macos
