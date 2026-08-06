#!/usr/bin/env bash
# shellcheck disable=SC2154

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

check-builder-cache() {
  local checksum
  checksum="$(openssl dgst -sha256 "${PWD}/build-aux/com.obsproject.Studio.json")"

  local manifest_hash
  read -r _ manifest_hash <<< "${checksum}"

  # When provided a cache key, Flatpak-builder automatically adds the
  # build architecture to the key in case multiple architectures are built.
  # Otherwise it generates a cache-key based on the pattern:
  # 'flatpak-builder-<architecture>-<first 20 characters of manifest-hash>
  cache_key="$(gh cache list \
    --ref "refs/heads/master" \
    --key "flatpak-builder-x86_64-${manifest_hash:0:20}" \
    --limit 1 --json key --jq '.[0].key')"

  {
    if [[ -n "${cache_key}" ]]; then
      echo "cache-hit=true"
    else
      echo "cache-hit=false"
    fi
  }  >> "${GITHUB_OUTPUT}"
}

check-builder-cache
