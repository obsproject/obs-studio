#pragma once

#include "OBSQTDisplay.hpp"

#include <QFrame>
#include <QVBoxLayout>

class OBSSourceWidgetView;

class OBSSourceWidget : public QFrame {
	Q_OBJECT

private:
	OBSSourceWidgetView *sourceView = nullptr;
	QVBoxLayout *layout;

	double fixedAspectRatio;

	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);

public:
	OBSSourceWidget(QWidget *parent);
	OBSSourceWidget(QWidget *parent, obs_source_t *source);
	~OBSSourceWidget();

	void setFixedAspectRatio(double ratio);
	void setSource(obs_source_t *source);

	void resizeSourceView();
};

class OBSSourceWidgetView : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSWeakSourceAutoRelease weakSource;

	static void OBSRender(void *data, uint32_t cx, uint32_t cy);

	QRect prevGeometry;

	int32_t sourceWidth_;
	int32_t sourceHeight_;

public:
	OBSSourceWidgetView(OBSSourceWidget *parent, obs_source_t *source);
	~OBSSourceWidgetView();

	void setSource(obs_source_t *source);
	void setSourceWidth(int width);
	void setSourceHeight(int height);
	int sourceWidth() { return sourceWidth_; }
	int sourceHeight() { return sourceHeight_; }

	OBSSource GetSource();

signals:
	void viewReady();
};
