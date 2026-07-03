#!/usr/bin/env bash
# shellcheck disable=SC2154

# TOTP generation code inspired by https://thenybble.de/posts/calculating-totp/

set -o errexit
set -o nounset
set -o pipefail

: "${CI:?}"
if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi

get-time-offset() {
  local -i start_time=0
  local -i end_time=0

  start_time="$(date -u '+%s')"
  local steam_response
  steam_response="$(curl \
    --silent \
    --request 'POST' \
    --header 'Content-Length: 0' \
    https://api.steampowered.com/ITwoFactorService/QueryTime/v1/)"

  local -i server_time=0
  server_time="$(jq -r '.response.server_time' <<< "${steam_response}")"

  if ! [[ "${server_time}" =~ ^[0-9]+$ ]]; then
    echo '::error::Malformed response from Steam time query service.'
    return 1
  fi
  end_time="$(date -u '+%s')"
  time_offset="$(( server_time - end_time ))"
}

generate-hmac() {
  local -i start_time=0
  local -i time_counter=0
  local time_counter_hex
  local hex_secret
  local hmac_value
  local last_byte
  local -i offset_value=0
  local hmac_slice

  # Generate counter by taking current timestamp and dividing by 30
  start_time="$(date -u '+%s')"
  start_time="$(( start_time + time_offset ))"
  time_counter="$(( start_time / 30 ))"

  # Counter has to be 8-byte signed integer per spec, convert via printf
  time_counter_hex="$(printf '%.16x' "${time_counter}")"

  # Convert base64-encoded key to hexadecimal value
  {
    set +x > /dev/null
    hex_secret="$(echo -n "${STEAM_SECRET}" | base64 -d | xxd -p)"
    echo "::add-mask::${hex_secret}"

    if [[ -n "${RUNNER_DEBUG:-}" ]]; then set -x; fi
  }

  # Convert counter to binary via xxd, hash using openssl and hex key
  hmac_value="$(echo -n "${time_counter_hex}" \
    | xxd -r -p \
    | openssl mac -digest sha1 -macopt hexkey:"${hex_secret}" HMAC)"

  # HOTP requires different substring of hash behavior based on last 4 bits of hash.
  # Convert last hex value (4 bits) to number to get offset
  last_byte="${hmac_value:39}"
  offset_value="$(( 16#${last_byte} ))"

  # Multiply offset by 2 to get start slice, then extract 8 bytes
  offset_value="$(( offset_value * 2 ))"
  hmac_slice="${hmac_value:"${offset_value}":8}"
  echo "::add-mask::${hmac_slice}"

  # Convert extracted bytes to decimal
  hmac_result="$(( 16#${hmac_slice} & 0x7FFFFFFF ))"
  echo "::add-mask::${hmac_result}"
}

generate-code() {
  # Code based on https://github.com/DoctorMcKay/node-steam-totp
  # index.js#L54-L60

  local char_list='23456789BCDFGHJKMNPQRTVWXY'
  local -i char_length="${#char_list}"

  for _ in {1..5}; do
    char_slice="$(( hmac_result % char_length ))"
    char_code+="${char_list:${char_slice}:1}"
    hmac_result="$(( hmac_result / char_length ))"
  done

  echo "::add-mask::${char_code}"
}

setup-steam-totp() {
  local secret_length="${#STEAM_SECRET}"

  if (( secret_length != 28 )) && [[ "${STEAM_SECRET:0-1}" != '=' ]]; then
    echo '::error::Invalid base64 encoded secret supplied.'
    return 1
  fi

  local -i time_offset=0
  get-time-offset

  local -i hmac_result
  generate-hmac

  local char_code=''
  generate-code

  echo "code=${char_code}" >> "${GITHUB_OUTPUT}"
}

setup-steam-totp
