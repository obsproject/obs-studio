#!/bin/bash
cd "$(dirname "$0")"
find . \( -name '*.cpp' -o -name '*.c' -o -name '*.h' -o -name '*.hpp' -o -name '*.re2c' \) -exec astyle --style=stroustrup --attach-extern-c --break-blocks --pad-header --pad-paren-out --unpad-paren --add-brackets --keep-one-line-blocks --keep-one-line-statements --convert-tabs --align-pointer=type --align-reference=type --suffix=none --lineend=linux --max-code-length=180 {} \;
