#pragma once

#include <QCheckBox>

class MuteCheckBox : public QCheckBox {
	Q_OBJECT

public:
	MuteCheckBox(QWidget *parent = nullptr) : QCheckBox(parent)
	{
		setTristate(true);
		setProperty("class", "indicator-mute");
	}

protected:
	/* While we need it to be tristate internally, we don't want a user being
	 * able to manually get into the partial state. */
	void nextCheckState() override
	{
		if (checkState() != Qt::Checked)
			setCheckState(Qt::Checked);
		else
			setCheckState(Qt::Unchecked);
	}
};
