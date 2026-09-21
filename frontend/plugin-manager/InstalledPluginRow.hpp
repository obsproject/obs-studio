/******************************************************************************
    Copyright (C) 2026 by Warchamp7 <warchamp7@obsproject.com>

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

#include <plugin-manager/PluginManagerWindow.hpp>

#include <Idian/Row.hpp>

namespace OBS {

class InstalledPluginRow : public idian::Row {
	Q_OBJECT

public:
	InstalledPluginRow(QWidget *parent, const PluginManagerWindow::Entry &entry);

signals:
	void toggleChanged();
	void trashClicked();

private:
	static const QIcon &getWarningIcon();
	static const QIcon &getTrashIcon();
};
} // namespace OBS
