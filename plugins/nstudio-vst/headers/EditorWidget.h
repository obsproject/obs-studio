/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

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

#ifndef OBS_STUDIO_EDITORDIALOG_H
#define OBS_STUDIO_EDITORDIALOG_H

#include <QWidget>

#if defined(_WIN32)
#include <QWindow>
#include <Windows.h>
#elif defined(__linux__)
#include <QWindow>
#include <xcb/xcb.h>
#endif

#include "aeffectx.h"
#include "VSTPlugin.h"

class VSTPlugin;

class VstRect {

public:
	short top;
	short left;
	short bottom;
	short right;
};

class EditorWidget : public QWidget {

	VSTPlugin *plugin;

#if defined(__APPLE__)
	QWidget *cocoaViewContainer = NULL;
#elif defined(_WIN32)
	HWND windowHandle = NULL;
#endif

public:
	EditorWidget(QWidget *parent, VSTPlugin *plugin);
	void buildEffectContainer(AEffect *effect);
	void closeEvent(QCloseEvent *event) override;
	void handleResizeRequest(int width, int height);
};

#endif // OBS_STUDIO_EDITORDIALOG_H
