#!/bin/zsh
 
arch_name="$(uname -m)"

# When the script is launched from Steam, it'll be run through Rosetta.
# Manually override arch to arm64 in that case.
if [ "$(sysctl -in sysctl.proc_translated)" = "1" ]; then
    arch_name="arm64"
fi

# Allow users to force Rosetta
if [[ "$@" =~ \-\-intel ]]; then
    arch_name="x86_64"
fi

# legacy app installation
if [ -d OBS.app ]; then
    exec open OBS.app -W --args "$@"
fi

if [ "${arch_name}" = "x86_64" ]; then
    exec open x86/OBS.app -W --args "$@"
elif [ "${arch_name}" = "arm64" ]; then
    exec open arm64/OBS.app -W --args "$@"
else
    echo "Unknown architecture: ${arch_name}"
fi
