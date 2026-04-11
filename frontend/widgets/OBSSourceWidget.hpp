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

	static constexpr int FALLBACK_WIDTH = 96;
	static constexpr int FALLBACK_HEIGHT = 54;

public:
	OBSSourceWidget(QWidget *parent);
	OBSSourceWidget(QWidget *parent, obs_source_t *source);
	~OBSSourceWidget();

	void setFixedAspectRatio(double ratio);
	void setSource(obs_source_t *source);

	void resizeSourceView();

	QSize minimumSizeHint();
	QSize sizeHint();
	bool hasHeightForWidth();
	int heightForWidth(int width);

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
};

class OBSSourceWidgetView : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSWeakSource weakSource = nullptr;

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
