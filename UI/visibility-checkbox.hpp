#include <QCheckBox>
#include <QPixmap>

class QPaintEvernt;

class VisibilityCheckBox : public QCheckBox {
	Q_OBJECT

	QPixmap checkedImage;
	QPixmap uncheckedImage;

public:
	VisibilityCheckBox();

protected:
	void paintEvent(QPaintEvent *event) override;
};
