#include "visibility-checkbox.hpp"
#include "icons.hpp"

VisibilityCheckBox::VisibilityCheckBox(QWidget *parent) : OBSCheckBox(parent)
{
	SetCheckedIcon(GetIcon(":/res/images/visible.svg"));
	SetUncheckedIcon(
		GetIcon(":/res/images/invisible.svg", IconType::Disabled));
}
