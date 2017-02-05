#!/usr/bin/env bash

# Fix permissions on CEF
chmod 744 "/Library/Application Support/obs-studio/plugins/obs-browser/bin/CEF.app/Contents/Info.plist"
chmod 744 "/Library/Application Support/obs-studio/plugins/obs-browser/bin/CEF.app/Contents/Frameworks/CEF Helper.app/Contents/Info.plist"
