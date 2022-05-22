/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

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
*****************************************************************************/

#include "headers/EditorWidget.h"
#include <QCloseEvent>

EditorWidget::EditorWidget(QWidget *parent, VSTPlugin *plugin)
	: QWidget(parent), plugin(plugin)
{
	setWindowFlags(this->windowFlags() |= Qt::MSWindowsFixedSizeDialogHint);
}

void EditorWidget::closeEvent(QCloseEvent *event)
{
	plugin->onEditorClosed();
	UNUSED_PARAMETER(event);
}
