#include "checkbox.hpp"
#include <QPainter>

OBSCheckBox::OBSCheckBox(QWidget *parent) : QCheckBox(parent) {}

void OBSCheckBox::SetCheckedIcon(QIcon icon)
{
	checkedIcon = icon;
}

void OBSCheckBox::SetUncheckedIcon(QIcon icon)
{
	uncheckedIcon = icon;
}

void OBSCheckBox::paintEvent(QPaintEvent *event)
{
	QIcon icon = isChecked() ? checkedIcon : uncheckedIcon;

	if (icon.isNull()) {
		QCheckBox::paintEvent(event);
		return;
	}

	QPainter painter(this);
	icon.paint(&painter, 0, 0, size().width(), size().height());
}
