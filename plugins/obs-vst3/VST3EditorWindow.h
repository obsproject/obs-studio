/*  Copyright (c) 2025 pkv <pkv@obsproject.com>
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#pragma once
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include <string>
#ifdef __linux__
#include "editor/linux/RunLoopImpl.h"
#endif

class VST3EditorWindow {
public:
#ifdef __linux__
	VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title, Display *display, RunLoopImpl *runloop);
#else
	VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title);
#endif
	~VST3EditorWindow();

	bool create(int width, int height);
	void show();
	void close();
	// Our design of the VST3 GUI is such that it is hidden when one clicks on the x (close), except for linux.
	// The reason is that we want to retain position and size of the GUI. But this can create a desync of the
	// visibility state. So we need to retrieve whether the GUI was closed with x.
	bool getClosedState();
#if defined(__APPLE__)
	void setClosedState(bool closed);
#endif
	class Impl;
	Impl *_impl;
};
