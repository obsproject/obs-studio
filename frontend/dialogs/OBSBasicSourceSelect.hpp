/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "ui_OBSBasicSourceSelect.h"

#include <utility/undo_stack.hpp>
#include <widgets/OBSBasic.hpp>

#include <obs.hpp>

#include <QDialog>

class OBSBasicSourceSelect : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::OBSBasicSourceSelect> ui;
	const char *id;
	undo_stack &undo_s;

	static bool EnumSources(void *data, obs_source_t *source);
	static bool EnumGroups(void *data, obs_source_t *source);

	static void OBSSourceRemoved(void *data, calldata_t *calldata);
	static void OBSSourceAdded(void *data, calldata_t *calldata);

private slots:
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

	void SourceAdded(OBSSource source);
	void SourceRemoved(OBSSource source);

public:
	OBSBasicSourceSelect(OBSBasic *parent, const char *id, undo_stack &undo_s);

	OBSSource newSource;

	static void SourcePaste(SourceCopyInfo &info, bool duplicate);
};
