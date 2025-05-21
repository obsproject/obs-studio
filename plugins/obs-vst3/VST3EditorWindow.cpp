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

#include "VST3EditorWindow.h"
#ifdef __linux__
#include "VST3HostApp.h"
#endif

#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>

#include <QCloseEvent>
#include <QEvent>
#include <QResizeEvent>
#include <QString>
#include <QWindow>

#ifdef __linux__
#include <QGuiApplication>
#include <QTimer>
#endif

#ifdef _WIN32
#include <QByteArray>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>

constexpr int obsIconId = 101;
#endif

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

#ifdef __linux__
VST3HostApp *getHostApp() noexcept;
#endif

using namespace Steinberg;

namespace {

bool sameSize(const ViewRect &lhs, const ViewRect &rhs)
{
	return lhs.getWidth() == rhs.getWidth() && lhs.getHeight() == rhs.getHeight();
}

void reportError(const char *message)
{
	std::fprintf(stderr, "VST3 editor error: %s\n", message);
}

} // namespace

#ifdef _WIN32
HMODULE getCurrentModule()
{
	HMODULE module = nullptr;

	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			   reinterpret_cast<LPCWSTR>(&getCurrentModule), &module);

	return module;
}
#endif

class VST3EditorWindow::PlugFrameImpl : public IPlugFrame {
public:
	explicit PlugFrameImpl(VST3EditorWindow *window) : window_(window) {}
	tresult PLUGIN_API resizeView(IPlugView *view, ViewRect *newSize) override
	{
		if (!window_ || !view || view != window_->view_ || !newSize || newSize->getWidth() <= 0 ||
		    newSize->getHeight() <= 0) {
			return kInvalidArgument;
		}

		return window_->resizeFromPlugin(*newSize);
	}
	tresult PLUGIN_API queryInterface(const TUID _iid, void **obj) override
	{
		if (FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
			*obj = static_cast<FUnknown *>(static_cast<IPlugFrame *>(this));
			return kResultOk;
		}
		if (FUnknownPrivate::iidEqual(_iid, IPlugFrame::iid)) {
			*obj = static_cast<IPlugFrame *>(this);
			return kResultOk;
		}
#ifdef __linux__
		if (getHostApp() && FUnknownPrivate::iidEqual(_iid, Linux::IRunLoop::iid)) {
			return getHostApp()->queryInterface(_iid, obj);
		}
#endif
		*obj = nullptr;
		return kNoInterface;
	}
	// refcounting does not matter here, cf SDK
	uint32_t PLUGIN_API addRef() override { return 1; }
	uint32_t PLUGIN_API release() override { return 1; }

private:
	VST3EditorWindow *window_;
};

VST3EditorWindow::VST3EditorWindow(IPlugView *view, const std::string &title) : QWidget(nullptr), view_(view)
{
	viewContainer_ = new QWidget(this);

	resizable_ = view_->canResize() == kResultTrue;

	Qt::WindowFlags flags = Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
				Qt::WindowMinimizeButtonHint;

	if (resizable_) {
		flags |= Qt::WindowMaximizeButtonHint;
	}

	setWindowFlags(flags);
	setWindowTitle(QString::fromUtf8(title.data(), static_cast<qsizetype>(title.size())));

#ifdef __APPLE__
	setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif
#ifdef _WIN32
	if (const HICON icon = LoadIconW(getCurrentModule(), MAKEINTRESOURCEW(obsIconId))) {
		setWindowIcon(QIcon(QPixmap::fromImage(QImage::fromHICON(icon))));
	}
#endif
#ifdef __linux__
	resizeLinuxTimer_ = new QTimer(this);
	resizeLinuxTimer_->setSingleShot(true);
	resizeLinuxTimer_->setInterval(10);
	connect(resizeLinuxTimer_, &QTimer::timeout, this, [this] { handleLinuxResize(); });
#endif
}

VST3EditorWindow::~VST3EditorWindow() = default;

bool VST3EditorWindow::create(int width, int height)
{
	if (!view_ || attached_ || width <= 0 || height <= 0) {
		return false;
	}

	ViewRect initialSize(0, 0, width, height);
	view_->checkSizeConstraint(&initialSize);
	const QSize qtInitialSize = vst3ToQtSize(initialSize);
	if (resizable_) {
		resize(qtInitialSize);
	} else {
		setFixedSize(qtInitialSize);
	}

	viewContainer_->setGeometry(0, 0, qtInitialSize.width(), qtInitialSize.height());

	QWidget::create();
#ifdef __APPLE__
	if (resizable_) {
		setMacContentAspectRatio(qtInitialSize);
	}
#endif
	const WId nativeParentId = viewContainer_->winId();
	if (!windowHandle() || !nativeParentId) {
		reportError("failed to create the native window");
		return false;
	}

	connect(windowHandle(), &QWindow::screenChanged, this, [this] { handleScaleChange(); });

	frame_ = std::make_unique<PlugFrameImpl>(this);
	view_->setFrame(frame_.get());
	void *nativeParent = reinterpret_cast<void *>(nativeParentId);
#ifdef _WIN32
	const FIDString platformType = kPlatformTypeHWND;
#elif defined(__APPLE__)
	const FIDString platformType = kPlatformTypeNSView;
#else
	const FIDString platformType = kPlatformTypeX11EmbedWindowID;
#endif

	if (view_->attached(nativeParent, platformType) != kResultOk) {
		view_->setFrame(nullptr);
		frame_.reset();
		reportError("failed to attach the plug-in view");

		return false;
	}

	attached_ = true;

	ViewRect windowSize = qtToVst3Rect(viewContainer_->size());
	view_->onSize(&windowSize);

	(void)updateContentScaleFactor();

	return true;
}

void VST3EditorWindow::show()
{
	if (!attached_) {
		return;
	}

	QWidget::show();
	raise();
	activateWindow();

	wasClosed_ = false;

	handleScaleChange();
}

void VST3EditorWindow::close()
{
	hide();
}

bool VST3EditorWindow::getClosedState() const
{
	return wasClosed_;
}

bool VST3EditorWindow::event(QEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0) && (defined(_WIN32) || defined(__APPLE__))
	if (event->type() == QEvent::DevicePixelRatioChange) {
		handlingDpiChange_ = true;

		const bool handled = QWidget::event(event);
		const bool scaleChanged = updateContentScaleFactor();

		handlingDpiChange_ = false;

		if (scaleChanged && attached_) {
			ViewRect actualRect = qtToVst3Rect(viewContainer_->size());
			view_->onSize(&actualRect);
		}

		return handled;
	}
#endif
	return QWidget::event(event);
}

void VST3EditorWindow::closeEvent(QCloseEvent *event)
{
	wasClosed_ = true;
	hide();
	event->ignore();
}

void VST3EditorWindow::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	viewContainer_->setGeometry(0, 0, event->size().width(), event->size().height());

	if (!view_ || !attached_ || resizingFromPlugin_ || correctingHostResize_ || handlingDpiChange_) {
		return;
	}

#ifdef _WIN32
	ViewRect actualRect = qtToVst3Rect(viewContainer_->size());
	view_->onSize(&actualRect);
#elif defined(__APPLE__)
	ViewRect requested = qtToVst3Rect(event->size());
	ViewRect constrained = requested;

	if (resizable_) {
		view_->checkSizeConstraint(&constrained);
	}

	if (!sameSize(requested, constrained)) {
		correctingHostResize_ = true;
		resize(vst3ToQtSize(constrained));
		correctingHostResize_ = false;
	}

	ViewRect actualRect = qtToVst3Rect(viewContainer_->size());
	view_->onSize(&actualRect);
#else
	resizeLinuxTimer_->start();
#endif
}

#ifdef _WIN32
bool VST3EditorWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
	if (eventType != QByteArrayLiteral("windows_generic_MSG")) {
		return QWidget::nativeEvent(eventType, message, result);
	}

	auto *msg = static_cast<MSG *>(message);

	if (!view_ || !attached_ || !resizable_ || msg->hwnd != reinterpret_cast<HWND>(winId()) ||
	    msg->message != WM_SIZING) {
		return QWidget::nativeEvent(eventType, message, result);
	}

	auto *windowRect = reinterpret_cast<RECT *>(msg->lParam);
	if (!windowRect) {
		return QWidget::nativeEvent(eventType, message, result);
	}

	const LONG style = GetWindowLong(msg->hwnd, GWL_STYLE);
	const LONG exStyle = GetWindowLong(msg->hwnd, GWL_EXSTYLE);

	RECT nonClientRect{0, 0, 0, 0};
	AdjustWindowRectEx(&nonClientRect, style, FALSE, exStyle);

	const int extraWidth = nonClientRect.right - nonClientRect.left;
	const int extraHeight = nonClientRect.bottom - nonClientRect.top;

	const int windowWidth = windowRect->right - windowRect->left;
	const int windowHeight = windowRect->bottom - windowRect->top;

	ViewRect constrained(0, 0, windowWidth - extraWidth, windowHeight - extraHeight);
	view_->checkSizeConstraint(&constrained);

	const int newWindowWidth = constrained.getWidth() + extraWidth;
	const int newWindowHeight = constrained.getHeight() + extraHeight;

	switch (msg->wParam) {
	case WMSZ_LEFT:
	case WMSZ_TOPLEFT:
	case WMSZ_BOTTOMLEFT:
		windowRect->left = windowRect->right - newWindowWidth;
		break;

	default:
		windowRect->right = windowRect->left + newWindowWidth;
		break;
	}

	switch (msg->wParam) {
	case WMSZ_TOP:
	case WMSZ_TOPLEFT:
	case WMSZ_TOPRIGHT:
		windowRect->top = windowRect->bottom - newWindowHeight;
		break;

	default:
		windowRect->bottom = windowRect->top + newWindowHeight;
		break;
	}

	if (result) {
		*result = TRUE;
	}

	return true;
}
#endif

#ifdef __APPLE__
void VST3EditorWindow::setMacContentAspectRatio(const QSize &size)
{
	if (size.isEmpty()) {
		return;
	}

	NSView *nativeView = (__bridge NSView *)(reinterpret_cast<void *>(winId()));

	if (!nativeView || !nativeView.window) {
		return;
	}

	nativeView.window.contentAspectRatio =
		NSMakeSize(static_cast<CGFloat>(size.width()), static_cast<CGFloat>(size.height()));
}
#endif

qreal VST3EditorWindow::vst3CoordinateScaleFactor() const
{
#if defined(_WIN32)
	return windowHandle() ? windowHandle()->devicePixelRatio() : devicePixelRatioF();
#elif defined(__linux__)
	if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
		return windowHandle() ? windowHandle()->devicePixelRatio() : devicePixelRatioF();
	}
#endif
	return 1.0;
}

QSize VST3EditorWindow::vst3ToQtSize(const ViewRect &rect) const
{
	const qreal scale = vst3CoordinateScaleFactor();
	const int width = std::max(1, static_cast<int>(std::lround(rect.getWidth() / scale)));
	const int height = std::max(1, static_cast<int>(std::lround(rect.getHeight() / scale)));

	return {width, height};
}

ViewRect VST3EditorWindow::qtToVst3Rect(const QSize &size) const
{
	const qreal scale = vst3CoordinateScaleFactor();
	const auto width = static_cast<int32>(std::lround(size.width() * scale));
	const auto height = static_cast<int32>(std::lround(size.height() * scale));

	return {0, 0, std::max<int32>(1, width), std::max<int32>(1, height)};
}

tresult PLUGIN_API VST3EditorWindow::resizeFromPlugin(const ViewRect &rect)
{
	if (!view_ || resizeViewRecursionGuard_ || rect.getWidth() <= 0 || rect.getHeight() <= 0) {
		return kResultFalse;
	}

	const QSize targetSize = vst3ToQtSize(rect);

	if (viewContainer_->size() == targetSize) {
		return kResultTrue;
	}

	resizeViewRecursionGuard_ = true;
	resizingFromPlugin_ = true;

	if (resizable_) {
		resize(targetSize);
	} else {
		setFixedSize(targetSize);
	}

	resizingFromPlugin_ = false;

	ViewRect actualRect = qtToVst3Rect(viewContainer_->size());
	view_->onSize(&actualRect);

	resizeViewRecursionGuard_ = false;

	return kResultTrue;
}

#ifdef __linux__
// TODO: improve X11 handling. It gave me a lot of headaches. Currently we use a throttle timer to call the plugin
// onSize. Without it I had repaint issues. Known BUG: currently there's still repaint issues with LSP VST3s when
// they are resized with their inner handle but not when the container is resized. Other VST3s on linux are fine.
void VST3EditorWindow::handleLinuxResize()
{
	if (!view_ || !attached_ || resizingFromPlugin_ || correctingHostResize_) {
		return;
	}

	const QSize requestedQtSize = viewContainer_->size();
	ViewRect requestedRect = qtToVst3Rect(requestedQtSize);
	ViewRect constrainedRect = requestedRect;

	tresult constraintResult = kResultFalse;

	if (resizable_) {
		constraintResult = view_->checkSizeConstraint(&constrainedRect);
	}

	if (constraintResult == kResultTrue && !sameSize(requestedRect, constrainedRect)) {
		const QSize constrainedQtSize = vst3ToQtSize(constrainedRect);

		correctingHostResize_ = true;
		if (constrainedRect.getWidth() > requestedRect.getWidth() &&
		    constrainedRect.getHeight() > requestedRect.getHeight()) {
			setMinimumSize(constrainedQtSize);
		} else {
			resize(constrainedQtSize);
		}
		correctingHostResize_ = false;

		ViewRect actualRect = qtToVst3Rect(viewContainer_->size());
		view_->onSize(&actualRect);
		return;
	}
	view_->onSize(&requestedRect);
}
#endif

bool VST3EditorWindow::updateContentScaleFactor()
{
#if defined(_WIN32) || defined(__APPLE__)
	const qreal scale = devicePixelRatioF();

	if (contentScaleFactor_ == scale) {
		return false;
	}

	contentScaleFactor_ = scale;

	FUnknownPtr<IPlugViewContentScaleSupport> scaleSupport(view_);

	if (scaleSupport) {
		scaleSupport->setContentScaleFactor(static_cast<IPlugViewContentScaleSupport::ScaleFactor>(scale));
	}

	return true;
#else
	return false;
#endif
}

void VST3EditorWindow::handleScaleChange()
{
	if (updateContentScaleFactor() && attached_) {
		ViewRect currentSize = qtToVst3Rect(viewContainer_->size());
		view_->onSize(&currentSize);
	}
}
