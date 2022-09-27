/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "../headers/VSTPlugin.h"

AEffect *VSTPlugin::loadEffect() {
  AEffect *newEffect = NULL;

  // Create a path to the bundle
  CFStringRef pluginPathStringRef = CFStringCreateWithCString(
      NULL, pluginPath.c_str(), kCFStringEncodingUTF8);
  CFURLRef bundleUrl = CFURLCreateWithFileSystemPath(
      kCFAllocatorDefault, pluginPathStringRef, kCFURLPOSIXPathStyle, true);

  if (bundleUrl == NULL) {
    blog(LOG_WARNING, "Couldn't make URL reference for VST plug-in");
    return NULL;
  }

  // Open the bundle
  bundle = CFBundleCreate(kCFAllocatorDefault, bundleUrl);
  if (bundle == NULL) {
    blog(LOG_WARNING, "Couldn't create VST bundle reference.");
    CFRelease(pluginPathStringRef);
    CFRelease(bundleUrl);
    return NULL;
  }

  vstPluginMain mainEntryPoint = NULL;
  mainEntryPoint = (vstPluginMain)CFBundleGetFunctionPointerForName(
      bundle, CFSTR("VSTPluginMain"));

  // VST plugins previous to the 2.4 SDK used main_macho for the
  // entry point name.
  if (mainEntryPoint == NULL) {
    mainEntryPoint = (vstPluginMain)CFBundleGetFunctionPointerForName(
        bundle, CFSTR("main_macho"));
  }

  if (mainEntryPoint == NULL) {
    blog(LOG_WARNING, "Couldn't get a pointer to plug-in's main()");
    CFBundleUnloadExecutable(bundle);
    CFRelease(bundle);
    return NULL;
  }

  newEffect = mainEntryPoint(hostCallback_static);
  if (newEffect == NULL) {
    blog(LOG_WARNING, "VST Plug-in's main() returns null.");
    CFBundleUnloadExecutable(bundle);
    CFRelease(bundle);
    return NULL;
  }

  newEffect->user = this;

  // Clean up
  CFRelease(pluginPathStringRef);
  CFRelease(bundleUrl);

  return newEffect;
}

void VSTPlugin::unloadLibrary() {
  if (bundle) {
    CFBundleUnloadExecutable(bundle);
    CFRelease(bundle);
  }
}