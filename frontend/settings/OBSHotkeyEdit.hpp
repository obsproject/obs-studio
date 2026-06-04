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

#pragma once

#include <obs.hpp>

#include <OBSApp.hpp>

#include <QLineEdit>

class OBSBasicSettings;
class QWidget;

static inline bool operator!=(const obs_key_combination_t &c1, const obs_key_combination_t &c2)
{
	return c1.modifiers != c2.modifiers || c1.key != c2.key;
}

static inline bool operator==(const obs_key_combination_t &c1, const obs_key_combination_t &c2)
{
	return !(c1 != c2);
}

class OBSHotkeyEdit : public QLineEdit {
	Q_OBJECT;

public:
	OBSHotkeyEdit(QWidget *parent, obs_key_combination_t original, OBSBasicSettings *settings)
		: QLineEdit(parent),
		  original(original),
		  settings(settings)
	{
		setAttribute(Qt::WA_InputMethodEnabled, false);
		setAttribute(Qt::WA_MacShowFocusRect, true);
		setStyle(App()->GetInvisibleCursorStyle());
		InitSignalHandler();
		ResetKey();
	}
	OBSHotkeyEdit(QWidget *parent = nullptr) : QLineEdit(parent), original({}), settings(nullptr)
	{
		setAttribute(Qt::WA_InputMethodEnabled, false);
		setAttribute(Qt::WA_MacShowFocusRect, true);
		setStyle(App()->GetInvisibleCursorStyle());
		InitSignalHandler();
		ResetKey();
	}

	obs_key_combination_t original;
	obs_key_combination_t key;
	OBSBasicSettings *settings;
	bool changed = false;

	void UpdateDuplicationState();
	bool hasDuplicate = false;
	QVariant inputMethodQuery(Qt::InputMethodQuery) const override;

protected:
	OBSSignal layoutChanged;
	QAction *dupeIcon = nullptr;

	void InitSignalHandler();
	void CreateDupeIcon();

	void keyPressEvent(QKeyEvent *event) override;
#ifdef __APPLE__
	void keyReleaseEvent(QKeyEvent *event) override;
#endif
	void mousePressEvent(QMouseEvent *event) override;

	void RenderKey();

public slots:
	void HandleNewKey(obs_key_combination_t new_key);
	void ReloadKeyLayout();
	void ResetKey();
	void ClearKey();

signals:
	void KeyChanged(obs_key_combination_t);
	void SearchKey(obs_key_combination_t);
};
