#pragma once

#include <QSpinBox>

class SilentUpdateSpinBox : public QSpinBox {
	Q_OBJECT

public slots:
	void setValueSilently(int val)
	{
		bool blocked = blockSignals(true);
		setValue(val);
		blockSignals(blocked);
	}
};
