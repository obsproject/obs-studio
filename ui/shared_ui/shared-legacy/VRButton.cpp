#include "VRButton.h"

VRButton::VRButton(const QString &text, QWidget *parent) : QPushButton(text, parent)
{
	setStyleSheet(VRTheme::buttonStyle());
	setCursor(Qt::PointingHandCursor);
}
