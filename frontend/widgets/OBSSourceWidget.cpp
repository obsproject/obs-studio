#include "OBSSourceView.hpp"

#include <OBSApp.hpp>
#include <utility/display-helpers.hpp>
#include <utility/platform.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QScreen>
#include <QWindow>

#include "moc_OBSSourceView.cpp"

OBSSourceWidget::OBSSourceWidget(QWidget *parent) : QFrame(parent), fixedAspectRatio(0.0)
{
	layout = new QVBoxLayout();
	setLayout(layout);

	layout->setContentsMargins(0, 0, 0, 0);
	setMinimumSize(QSize(240, 135));
}

OBSSourceWidget::OBSSourceWidget(QWidget *parent, obs_source_t *source) : OBSSourceWidget(parent)
{
	setSource(source);
}

void OBSSourceWidget::setFixedAspectRatio(double ratio)
{
	if (ratio > 0.0) {
		fixedAspectRatio = ratio;
	} else {
		fixedAspectRatio = 0;
	}
}

void OBSSourceWidget::setSource(obs_source_t *source)
{
	if (!sourceView) {
		sourceView = new OBSSourceWidgetView(this, source);
		layout->addWidget(sourceView);

		connect(sourceView, &OBSSourceWidgetView::viewReady, this, &OBSSourceWidget::resizeSourceView);
	}

	sourceView->setSource(source);
}

void OBSSourceWidget::resizeSourceView()
{
	if (!sourceView) {
		return;
	}

	if (sourceView->sourceWidth() <= 0 || sourceView->sourceHeight() <= 0) {
		return;
	}

	double aspectRatio = fixedAspectRatio > 0
				     ? fixedAspectRatio
				     : (double)sourceView->sourceWidth() / (double)sourceView->sourceHeight();

	// Widget only expands in one direction
	bool singleExpandDirection = (sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) !=
				     (sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag);

	int scaledWidth = std::floor(height() / aspectRatio);
	int scaledHeight = std::floor(width() / aspectRatio);

	if (fixedAspectRatio) {
		setMaximumWidth(QWIDGETSIZE_MAX);
		setMaximumHeight(scaledHeight);
	} else if (singleExpandDirection) {
		setMaximumWidth(QWIDGETSIZE_MAX);
		setMaximumHeight(QWIDGETSIZE_MAX);

		if ((sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) == QSizePolicy::ExpandFlag) {
			setMaximumHeight(scaledHeight);
		} else {
			setMaximumWidth(scaledWidth);
		}
	}
}

void OBSSourceWidget::moveEvent(QMoveEvent *event)
{
	resizeSourceView();

	if (sourceView) {
		QWindow *nativeWindow = sourceView->windowHandle();

		if (nativeWindow) {
			QPoint position = sourceView->mapTo(sourceView->nativeParentWidget(), QPoint());

			nativeWindow->setGeometry(QRect(position, sourceView->geometry().size()));
		}
	}

	QFrame::moveEvent(event);
}

void OBSSourceWidget::resizeEvent(QResizeEvent *event)
{
	resizeSourceView();
	QFrame::resizeEvent(event);
}

OBSSourceWidget::~OBSSourceWidget() {}

OBSSourceWidgetView::OBSSourceWidgetView(OBSSourceWidget *widget, obs_source_t *source_)
	: OBSQTDisplay(widget, Qt::Widget),
	  weakSource(OBSGetWeakRef(source_))
{
	//setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	OBSSource source = GetSource();
	if (source)
		obs_source_inc_showing(source);

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(GetDisplay(), OBSRender, this);
	};

	enum obs_source_type type = obs_source_get_type(source);
	bool drawable_type = type == OBS_SOURCE_TYPE_INPUT || type == OBS_SOURCE_TYPE_SCENE;

	uint32_t caps = obs_source_get_output_flags(source);
	if ((caps & OBS_SOURCE_VIDEO) != 0) {
		if (drawable_type) {
			connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);
		}
	}

	show();
}

OBSSourceWidgetView::~OBSSourceWidgetView()
{
	obs_display_remove_draw_callback(GetDisplay(), OBSRender, this);

	OBSSource source = GetSource();
	if (source)
		obs_source_dec_showing(source);
}

void OBSSourceWidgetView::setSourceWidth(int width)
{
	if (sourceWidth() == width)
		return;

	sourceWidth_ = width;
	emit viewReady();
}

void OBSSourceWidgetView::setSourceHeight(int height)
{
	if (sourceHeight() == height)
		return;

	sourceHeight_ = height;
	emit viewReady();
}

OBSSource OBSSourceWidgetView::GetSource()
{
	return OBSGetStrongRef(weakSource);
}

void OBSSourceWidgetView::OBSRender(void *data, uint32_t cx, uint32_t cy)
{
	OBSSourceWidgetView *widget = reinterpret_cast<OBSSourceWidgetView *>(data);

	OBSSource source = widget->GetSource();

	uint32_t sourceCX = std::max(obs_source_get_width(source), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(source), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	const bool previous = gs_set_linear_srgb(true);

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_source_video_render(source);

	gs_set_linear_srgb(previous);
	gs_projection_pop();
	gs_viewport_pop();

	widget->setSourceWidth(sourceCX);
	widget->setSourceHeight(sourceCY);
}

void OBSSourceWidgetView::setSource(obs_source_t *source)
{
	weakSource = OBSGetWeakRef(source);
}
