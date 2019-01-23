#pragma once

#include <QWidget>
#include <obs.hpp>

// Color format #AABBGGRR
#define GREY_COLOR_BACKGROUND 0xFF4C4C4C

class OBSQTDisplay : public QWidget {
	Q_OBJECT
	Q_PROPERTY(QColor displayBackgroudColor READ getDisplayGNDColor
			WRITE setDisplayGNDColor)

	OBSDisplay display;

	void CreateDisplay();

	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

signals:
	void DisplayCreated(OBSQTDisplay *window);
	void DisplayResized();

public:
	OBSQTDisplay(QWidget *parent = 0, Qt::WindowFlags flags = 0);

	virtual QPaintEngine *paintEngine() const override;

	inline obs_display_t *GetDisplay() const {return display;}

	uint32_t displayGNDColor = GREY_COLOR_BACKGROUND;

	QColor getDisplayGNDColor() const;
	void setDisplayGNDColor(QColor color);
	void updateDisplayGNDColor();

private:
	QColor m_displayGNDColor;
};
