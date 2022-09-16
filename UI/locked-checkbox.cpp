#include "locked-checkbox.hpp"
#include "icons.hpp"

LockedCheckBox::LockedCheckBox(QWidget *parent) : OBSCheckBox(parent)
{
	SetCheckedIcon(GetIcon(":/res/images/locked.svg"));
	SetUncheckedIcon(
		GetIcon(":/res/images/unlocked.svg", IconType::Disabled));
}
