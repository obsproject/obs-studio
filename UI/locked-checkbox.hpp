#include <QCheckBox>
#include <QPixmap>

class QPaintEvernt;

class LockedCheckBox : public QCheckBox {
	Q_OBJECT

	QPixmap lockedImage;
	QPixmap unlockedImage;

public:
	LockedCheckBox();

protected:
	void paintEvent(QPaintEvent *event) override;
};
