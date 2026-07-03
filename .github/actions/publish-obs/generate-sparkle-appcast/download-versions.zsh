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
if (( ${+RUNNER_DEBUG} )) setopt XTRACE

: ${CI:?}

get-feed-url() {
  local mount_point="/Volumes/obs-base-image"

  TRAPERR() {
    hdiutil detach ${mount_point}
    return 1
  }

  hdiutil attach \
    -noverify \
    -readonly \
    -noautoopen \
    -mountpoint ${mount_point} \
    ${base_image}

  sleep 2

  local info_plist="${mount_point}/OBS.app/Contents/Info.plist"

  if [[ ! -r ${info_plist} ]] {
    print '::error::No Info.plist found in specified base image.'
    return 1
  }

  typeset -g feed_url="$(plutil -extract SUFeedURL raw - < ${info_plist})"

  local arch_regex='.+/updates_([^_]+).+\.xml'
  local -a match=()
  local mbegin
  local mend
  if [[ ${feed_url} =~ ${arch_regex} ]] {
    typeset -g architecture=${match[1]//x86/x86_64}
  } else {
    print "::error::Unsupported feed url '${feed_url}'."
    return 1
  }

  unfunction TRAPERR
  hdiutil detach ${mount_point}
}

download-from-feed() {

  print "::group::Downloading Sparkle feed from '${feed_url}'..."
  curl \
    --location \
    --output-dir ${RUNNER_TEMP} \
    --remote-name \
    -- ${feed_url}
  print '::endgroup::'

  # The Xpath Xplained:
  #
  # //rss/channel/item              - Select every <item> node, under a
  #                                   <channel> node, under a <rss> node,
  #                                   which:
  # [*...]                          - Has a child node, which
  # [local-name()='channel']        - Has the local name "channel"
  #                                   (required to match the
  #                                   namespaced sparkle:channel node),
  #                                   which in turn has
  # [text()='<channel>']            - A text node that contains the
  #                                   content of inputs.channel
  # /enclosure/@url                 - Then select the "url" attribute of
  #                                   every <enclosure> node under
  #                                   these matching <item> nodes

  local xmllint_result="$(xmllint \
    -xpath "//rss/channel/item[*[local-name()='channel'][text()='${CHANNEL}']]/enclosure/@url" \
    ${RUNNER_TEMP}/${feed_url:t})"

  local -i deltas=0
  local -a match=()
  local mbegin
  local mend
  if [[ ${NUM_DELTAS} == (#b)(<->##) ]] {
    deltas=${match[1]}

    if (( deltas < 0 || deltas > 10 )) {
      print "::error:: Unsupported number of deltas '${deltas} (supported range: [1-10])."
      return 1
    }
  } else {
    print "::error::Provided delta count value '${NUM_DELTAS}' not a number."
    return 1
  }

  print '::group::Downloading prior OBS Studio versions...'
  local line
  local -i count=1
  local feed_item_url
  local -a match=()
  while read -r line; do
    if (( count > deltas )) {
      break
    }

    if [[ ${line} =~ url=\"(.+)\"$ ]] {
      feed_item_url="${match[1]}"
      print "Downloading prior OBS Version at '${feed_item_url}'..."
      curl \
        --location \
        --output-dir ${RUNNER_TEMP}/appcast_builds \
        --remote-name \
        -- ${feed_item_url}
      count+=1
    }
  done <<< "${xmllint_result}"
  print '::endgroup::'
}

download-versions() {
  local -a image_candidates=(${BASE_IMAGE}(N))

  if (( ! ${#image_candidates} )) {
    print "::error::No disk images found at '${BASE_IMAGE}'."
    exit 1
  }

  local base_image=${image_candidates[1]}

  local feed_url
  local architecture
  get-feed-url
  print "feed-url=${feed_url}" >> ${GITHUB_OUTPUT}
  print "architecture=${architecture}" >> ${GITHUB_OUTPUT}

  download-from-feed
  mv ${base_image} ${RUNNER_TEMP}/appcast_builds
}

download-versions
