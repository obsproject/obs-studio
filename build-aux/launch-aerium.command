#!/usr/bin/env bash
set -euo pipefail

package=$(cd "$(dirname "$0")" && pwd)
mkdir -p "${package}/runtime"
cd "${package}/runtime"
exec "${package}/Aerium.app/Contents/MacOS/Aerium" --portable --only-bundled-plugins "$@"