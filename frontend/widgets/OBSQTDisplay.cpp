#include "OBSQTDisplay.hpp"

#include <utility/display-helpers.hpp>
#include <utility/SurfaceEventFilter.hpp>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <obs-nix-platform.h>
#endif

#include <QWindow>
#ifdef ENABLE_WAYLAND
#include <QApplication>
#include <qpa/qplatformnativeinterface.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "moc_OBSQTDisplay.cpp"

static inline long long color_to_int(const QColor &color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
	};

	return shift(color.red(), 0) | shift(color.green(), 8) | shift(color.blue(), 16) | shift(color.alpha(), 24);
}

static inline QColor rgba_to_color(uint32_t rgba)
{
	return QColor::fromRgb(rgba & 0xFF, (rgba >> 8) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 24) & 0xFF);
}

static bool QTToGSWindow(QWindow *window, gs_window &gswindow)
{
	bool success = true;

#ifdef _WIN32
	gswindow.hwnd = (HWND)window->winId();
#elif __APPLE__
	gswindow.view = (id)window->winId();
#else
	switch (obs_get_nix_platform()) {
	case OBS_NIX_PLATFORM_X11_EGL:
		gswindow.id = window->winId();
		gswindow.display = obs_get_nix_platform_display();
		break;
#ifdef ENABLE_WAYLAND
	case OBS_NIX_PLATFORM_WAYLAND: {
		QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
		gswindow.display = native->nativeResourceForWindow("surface", window);
		success = gswindow.display != nullptr;
		break;
	}
#endif
	default:
		success = false;
		break;
	}
#endif
	return success;
}

OBSQTDisplay::OBSQTDisplay(QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);

	auto windowVisible = [this](bool visible) {
		if (!visible) {
#if !defined(_WIN32) && !defined(__APPLE__)
			display = nullptr;
#endif
			return;
		}

		if (!display) {
			CreateDisplay();
		} else {
			QSize size = GetPixelSize(this);
			obs_display_resize(display, size.width(), size.height());
		}
	};

	auto screenChanged = [this](QScreen *) {
		CreateDisplay();

		QSize size = GetPixelSize(this);
		obs_display_resize(display, size.width(), size.height());
	};

	connect(windowHandle(), &QWindow::visibleChanged, windowVisible);
	connect(windowHandle(), &QWindow::screenChanged, screenChanged);

	windowHandle()->installEventFilter(new SurfaceEventFilter(this));
}

QColor OBSQTDisplay::GetDisplayBackgroundColor() const
{
	return rgba_to_color(backgroundColor);
}

void OBSQTDisplay::SetDisplayBackgroundColor(const QColor &color)
{
	uint32_t newBackgroundColor = (uint32_t)color_to_int(color);

	if (newBackgroundColor != backgroundColor) {
		backgroundColor = newBackgroundColor;
		UpdateDisplayBackgroundColor();
	}
}

void OBSQTDisplay::UpdateDisplayBackgroundColor()
{
	obs_display_set_background_color(display, backgroundColor);
}

void OBSQTDisplay::CreateDisplay()
{
	if (display)
		return;

	if (destroying)
		return;

	if (!windowHandle()->isExposed())
		return;

	QSize size = GetPixelSize(this);

	gs_init_data info = {};
	info.cx = size.width();
	info.cy = size.height();
	info.format = GS_BGRA;
	info.zsformat = GS_ZS_NONE;

	if (!QTToGSWindow(windowHandle(), info.window))
		return;

	display = obs_display_create(&info, backgroundColor);

	emit DisplayCreated(this);
}

void OBSQTDisplay::paintEvent(QPaintEvent *event)
{
	CreateDisplay();

	QWidget::paintEvent(event);
}

void OBSQTDisplay::moveEvent(QMoveEvent *event)
{
	QWidget::moveEvent(event);

	OnMove();
}

bool OBSQTDisplay::nativeEvent(const QByteArray &, void *message, qintptr *)
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	switch (msg.message) {
	case WM_DISPLAYCHANGE:
		OnDisplayChange();
	}
#else
	UNUSED_PARAMETER(message);
#endif

	return false;
}

void OBSQTDisplay::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	CreateDisplay();

	if (isVisible() && display) {
		QSize size = GetPixelSize(this);
		obs_display_resize(display, size.width(), size.height());
	}

	emit DisplayResized();
}

QPaintEngine *OBSQTDisplay::paintEngine() const
{
	return nullptr;
}

void OBSQTDisplay::OnMove()
{
	if (display)
		obs_display_update_color_space(display);
}

void OBSQTDisplay::OnDisplayChange()
{
	if (display)
		obs_display_update_color_space(display);
}
