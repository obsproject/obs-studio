#pragma once

#include "obs.hpp"
#include <QSlider>
#include <QInputEvent>
#include <QtCore/QObject>
#include <QAccessibleWidget>
#include <QStyleOptionSlider>

class SliderIgnoreScroll : public QSlider {
	Q_OBJECT

public:
	SliderIgnoreScroll(QWidget *parent = nullptr);
	SliderIgnoreScroll(Qt::Orientation orientation,
			   QWidget *parent = nullptr);

protected:
	virtual void wheelEvent(QWheelEvent *event) override;
};

class VolumeSlider : public SliderIgnoreScroll {
	Q_OBJECT

public:
	obs_fader_t *fad;

	VolumeSlider(obs_fader_t *fader, QWidget *parent = nullptr);
	VolumeSlider(obs_fader_t *fader, Qt::Orientation orientation,
		     QWidget *parent = nullptr);
};

class VolumeAccessibleInterface : public QAccessibleWidget {

public:
	VolumeAccessibleInterface(QWidget *w);

	QVariant currentValue() const;
	void setCurrentValue(const QVariant &value);

	QVariant maximumValue() const;
	QVariant minimumValue() const;

	QVariant minimumStepSize() const;

private:
	VolumeSlider *slider() const;

protected:
	virtual QAccessible::Role role() const override;
	virtual QString text(QAccessible::Text t) const override;
};

class SliderIgnoreClick : public SliderIgnoreScroll {
public:
	inline SliderIgnoreClick(Qt::Orientation orientation,
				 QWidget *parent = nullptr)
		: SliderIgnoreScroll(orientation, parent)
	{
	}

protected:
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
	bool dragging = false;
};
