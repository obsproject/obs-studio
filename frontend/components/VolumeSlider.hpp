#pragma once

#include <components/AbsoluteSlider.hpp>

#include <obs.hpp>

class VolumeSlider : public AbsoluteSlider {
	Q_OBJECT

public:
	obs_fader_t *fad;

	VolumeSlider(obs_fader_t *fader, QWidget *parent = nullptr);
	VolumeSlider(obs_fader_t *fader, Qt::Orientation orientation, QWidget *parent = nullptr);

	bool getDisplayTicks() const;
	void setDisplayTicks(bool display);

private:
	bool displayTicks = false;
	QColor tickColor;

protected:
	virtual void paintEvent(QPaintEvent *event) override;
};
