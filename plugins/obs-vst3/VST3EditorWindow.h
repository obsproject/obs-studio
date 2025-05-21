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
#include <pluginterfaces/gui/iplugview.h>

#include <QSize>
#include <QWidget>

#include <memory>
#include <string>

class QCloseEvent;
class QEvent;
class QResizeEvent;
class QTimer;

class VST3EditorWindow : public QWidget {
public:
	VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title);
	~VST3EditorWindow() override;

	VST3EditorWindow(const VST3EditorWindow &) = delete;
	VST3EditorWindow &operator=(const VST3EditorWindow &) = delete;
	VST3EditorWindow(VST3EditorWindow &&) = delete;
	VST3EditorWindow &operator=(VST3EditorWindow &&) = delete;

	bool create(int width, int height);
	void show();
	void close();
	// The window is hidden rather than destroyed to preserve its size and position.
	bool getClosedState() const;

protected:
	bool event(QEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
#ifdef _WIN32
	bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif
private:
	class PlugFrameImpl;

	QSize vst3ToQtSize(const Steinberg::ViewRect &rect) const;
	Steinberg::ViewRect qtToVst3Rect(const QSize &size) const;
	qreal vst3CoordinateScaleFactor() const;

	Steinberg::tresult PLUGIN_API resizeFromPlugin(const Steinberg::ViewRect &rect);
	bool updateContentScaleFactor();
	void handleScaleChange();

	Steinberg::IPlugView *view_ = nullptr;
	std::unique_ptr<PlugFrameImpl> frame_;
	QWidget *viewContainer_ = nullptr;

	bool resizable_ = false;
	bool attached_ = false;
	bool wasClosed_ = false;

	bool resizingFromPlugin_ = false;
	bool correctingHostResize_ = false;
	bool resizeViewRecursionGuard_ = false;

	bool handlingDpiChange_ = false;
	qreal contentScaleFactor_ = 0.0;

#ifdef __APPLE__
	void setMacContentAspectRatio(const QSize &size);
#endif
#ifdef __linux__
	void handleLinuxResize();
	QTimer *resizeLinuxTimer_ = nullptr;
#endif
};
