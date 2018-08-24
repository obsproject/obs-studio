#pragma once

#include <QCheckBox>
#include <QPixmap>

class QPaintEvernt;

class LockedCheckBox : public QCheckBox {
	Q_OBJECT

	QPixmap lockedImage;
	QPixmap unlockedImage;

public:
	LockedCheckBox(QWidget *parent = 0);

protected:
	void paintEvent(QPaintEvent *event) override;
};
