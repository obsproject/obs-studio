#!/bin/zsh
 
arch_name="$(uname -m)"

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
