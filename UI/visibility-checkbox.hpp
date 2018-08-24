#pragma once

#include <QCheckBox>
#include <QPixmap>

class QPaintEvernt;

class VisibilityCheckBox : public QCheckBox {
	Q_OBJECT

	QPixmap checkedImage;
	QPixmap uncheckedImage;

public:
	VisibilityCheckBox(QWidget *parent = 0);

protected:
	void paintEvent(QPaintEvent *event) override;
};
