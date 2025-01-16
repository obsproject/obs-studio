/******************************************************************************
    Copyright (C) 2014-2015 by Ruwen Hahn <palana@stunned.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "OBSHotkeyEdit.hpp"
#include "OBSBasicSettings.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>
#include <util/dstr.hpp>

#include <QKeyEvent>

#include "moc_OBSHotkeyEdit.cpp"

void OBSHotkeyEdit::keyPressEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	obs_key_combination_t new_key;

	switch (event->key()) {
	case Qt::Key_Shift:
	case Qt::Key_Control:
	case Qt::Key_Alt:
	case Qt::Key_Meta:
		new_key.key = OBS_KEY_NONE;
		break;

#ifdef __APPLE__
	case Qt::Key_CapsLock:
		// kVK_CapsLock == 57
		new_key.key = obs_key_from_virtual_key(57);
		break;
#endif

	default:
		new_key.key = obs_key_from_virtual_key(event->nativeVirtualKey());
	}

	new_key.modifiers = TranslateQtKeyboardEventModifiers(event->modifiers());

	HandleNewKey(new_key);
}

QVariant OBSHotkeyEdit::inputMethodQuery(Qt::InputMethodQuery query) const
{
	if (query == Qt::ImEnabled) {
		return false;
	} else {
		return QLineEdit::inputMethodQuery(query);
	}
}

#ifdef __APPLE__
void OBSHotkeyEdit::keyReleaseEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	if (event->key() != Qt::Key_CapsLock)
		return;

	obs_key_combination_t new_key;

	// kVK_CapsLock == 57
	new_key.key = obs_key_from_virtual_key(57);
	new_key.modifiers = TranslateQtKeyboardEventModifiers(event->modifiers());

	HandleNewKey(new_key);
}
#endif

void OBSHotkeyEdit::mousePressEvent(QMouseEvent *event)
{
	obs_key_combination_t new_key;

	switch (event->button()) {
	case Qt::NoButton:
	case Qt::LeftButton:
	case Qt::RightButton:
	case Qt::AllButtons:
	case Qt::MouseButtonMask:
		return;

	case Qt::MiddleButton:
		new_key.key = OBS_KEY_MOUSE3;
		break;

#define MAP_BUTTON(i, j)                        \
	case Qt::ExtraButton##i:                \
		new_key.key = OBS_KEY_MOUSE##j; \
		break;
		MAP_BUTTON(1, 4)
		MAP_BUTTON(2, 5)
		MAP_BUTTON(3, 6)
		MAP_BUTTON(4, 7)
		MAP_BUTTON(5, 8)
		MAP_BUTTON(6, 9)
		MAP_BUTTON(7, 10)
		MAP_BUTTON(8, 11)
		MAP_BUTTON(9, 12)
		MAP_BUTTON(10, 13)
		MAP_BUTTON(11, 14)
		MAP_BUTTON(12, 15)
		MAP_BUTTON(13, 16)
		MAP_BUTTON(14, 17)
		MAP_BUTTON(15, 18)
		MAP_BUTTON(16, 19)
		MAP_BUTTON(17, 20)
		MAP_BUTTON(18, 21)
		MAP_BUTTON(19, 22)
		MAP_BUTTON(20, 23)
		MAP_BUTTON(21, 24)
		MAP_BUTTON(22, 25)
		MAP_BUTTON(23, 26)
		MAP_BUTTON(24, 27)
#undef MAP_BUTTON
	}

	new_key.modifiers = TranslateQtKeyboardEventModifiers(event->modifiers());

	HandleNewKey(new_key);
}

void OBSHotkeyEdit::HandleNewKey(obs_key_combination_t new_key)
{
	if (new_key == key || obs_key_combination_is_empty(new_key))
		return;

	key = new_key;

	changed = true;
	emit KeyChanged(key);

	RenderKey();
}

void OBSHotkeyEdit::RenderKey()
{
	DStr str;
	obs_key_combination_to_str(key, str);

	setText(QT_UTF8(str));
}

void OBSHotkeyEdit::ResetKey()
{
	key = original;

	changed = false;
	emit KeyChanged(key);

	RenderKey();
}

void OBSHotkeyEdit::ClearKey()
{
	key = {0, OBS_KEY_NONE};

	changed = true;
	emit KeyChanged(key);

	RenderKey();
}

void OBSHotkeyEdit::UpdateDuplicationState()
{
	if (!dupeIcon && !hasDuplicate)
		return;

	if (!dupeIcon)
		CreateDupeIcon();

	if (dupeIcon->isVisible() != hasDuplicate) {
		dupeIcon->setVisible(hasDuplicate);
		update();
	}
}

void OBSHotkeyEdit::InitSignalHandler()
{
	layoutChanged = {obs_get_signal_handler(), "hotkey_layout_change",
			 [](void *this_, calldata_t *) {
				 auto edit = static_cast<OBSHotkeyEdit *>(this_);
				 QMetaObject::invokeMethod(edit, "ReloadKeyLayout");
			 },
			 this};
}

void OBSHotkeyEdit::CreateDupeIcon()
{
	dupeIcon = addAction(settings->GetHotkeyConflictIcon(), ActionPosition::TrailingPosition);
	dupeIcon->setToolTip(QTStr("Basic.Settings.Hotkeys.DuplicateWarning"));
	QObject::connect(dupeIcon, &QAction::triggered, [=] { emit SearchKey(key); });
	dupeIcon->setVisible(false);
}

void OBSHotkeyEdit::ReloadKeyLayout()
{
	RenderKey();
}
