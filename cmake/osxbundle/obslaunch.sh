#!/bin/sh
cd "$(dirname "$0")"
cd ../Resources
exec ./obs "$@"

