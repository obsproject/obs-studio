#pragma once

#include <QCheckBox>

class QIcon;

class OBSCheckBox : public QCheckBox {
	Q_OBJECT

	QIcon checkedIcon;
	QIcon uncheckedIcon;

public:
	OBSCheckBox(QWidget *parent = nullptr);
	void SetCheckedIcon(QIcon icon);
	void SetUncheckedIcon(QIcon icon);

protected:
	virtual void paintEvent(QPaintEvent *event) override;
};
