#include "qt-display.hpp"
#include "qt-wrappers.hpp"
#include "display-helpers.hpp"
#include <QWindow>
#include <QScreen>
#include <QResizeEvent>
#include <QShowEvent>

OBSQTDisplay::OBSQTDisplay(QWidget *parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);

	auto windowVisible = [this] (bool visible)
	{
		if (!visible)
			return;

		if (!display) {
			CreateDisplay();
		} else {
			QSize size = GetPixelSize(this);
			obs_display_resize(display, size.width(), size.height());
		}
	};

	auto sizeChanged = [this] (QScreen*)
	{
		CreateDisplay();

		QSize size = GetPixelSize(this);
		obs_display_resize(display, size.width(), size.height());
	};

	connect(windowHandle(), &QWindow::visibleChanged, windowVisible);
	connect(windowHandle(), &QWindow::screenChanged, sizeChanged);
}

void OBSQTDisplay::CreateDisplay()
{
	if (display || !windowHandle()->isExposed())
		return;

	QSize size = GetPixelSize(this);

	gs_init_data info      = {};
	info.cx                = size.width();
	info.cy                = size.height();
	info.format            = GS_RGBA;
	info.zsformat          = GS_ZS_NONE;

	QTToGSWindow(winId(), info.window);

	display = obs_display_create(&info);

	emit DisplayCreated(this);
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

void OBSQTDisplay::paintEvent(QPaintEvent *event)
{
	CreateDisplay();

	QWidget::paintEvent(event);
}

QPaintEngine *OBSQTDisplay::paintEngine() const
{
	return nullptr;
}
