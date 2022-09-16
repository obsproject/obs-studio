#include "mute-checkbox.hpp"
#include "icons.hpp"

MuteCheckBox::MuteCheckBox(QWidget *parent) : OBSCheckBox(parent)
{
	SetCheckedIcon(GetIcon(":/res/images/mute.svg", IconType::Original));
	SetUncheckedIcon(GetIcon(":/settings/images/settings/audio.svg"));
}
