#pragma once

#include <slider-ignorewheel.hpp>

class AbsoluteSlider : public SliderIgnoreScroll {
	Q_OBJECT
	Q_PROPERTY(QColor tickColor READ getTickColor WRITE setTickColor DESIGNABLE true)

public:
	AbsoluteSlider(QWidget *parent = nullptr);
	AbsoluteSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

	QColor getTickColor() const;
	void setTickColor(QColor c);

	bool getDisplayTicks() const;
	void setDisplayTicks(bool display);

signals:
	void absoluteSliderHovered(int value);

protected:
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual bool eventFilter(QObject *obj, QEvent *event) override;

	int posToRangeValue(QMouseEvent *event);

	virtual void paintEvent(QPaintEvent *event) override;

private:
	bool dragging = false;
	bool displayTicks = false;

	QColor tickColor;
};
