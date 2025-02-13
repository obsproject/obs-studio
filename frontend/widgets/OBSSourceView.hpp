#pragma once

#include "OBSQTDisplay.hpp"

#include <QFrame>
#include <QVBoxLayout>

class OBSSourceWidgetView;

class OBSSourceWidget : public QFrame {
	Q_OBJECT

private:
	OBSSourceWidgetView *sourceView;
	QVBoxLayout *layout;

	double fixedAspectRatio;

	void handleViewReady();

	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);

public:
	OBSSourceWidget(QWidget *parent, obs_source_t *source_);
	~OBSSourceWidget();

	void setFixedAspectRatio(double ratio);

	void resizeSourceView();
};

class OBSSourceWidgetView : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSWeakSourceAutoRelease weakSource;

	static void OBSRender(void *data, uint32_t cx, uint32_t cy);

	QRect prevGeometry;
	int32_t _sourceWidth;
	int32_t _sourceHeight;

public:
	OBSSourceWidgetView(OBSSourceWidget *parent, obs_source_t *source_);
	~OBSSourceWidgetView();

	void setSourceWidth(int width);
	void setSourceHeight(int height);
	int sourceWidth() { return _sourceWidth; }
	int sourceHeight() { return _sourceHeight; }

	OBSSource GetSource();

signals:
	void viewReady();
};
