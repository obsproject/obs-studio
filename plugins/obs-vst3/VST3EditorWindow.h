/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
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
	// Our design of the VST3 GUI is such that it is hidden when one clicks on the x (close).
	// The reason is that we want to retain position and size of the GUI. But this can create a desync of the
	// visibility state. So we need to retrieve whether the GUI was closed with a click on x.
	bool getClosedState();
#if defined(__APPLE__)
	void setClosedState(bool closed);
#endif
	class Impl;
	Impl *impl_;
};
