/******************************************************************************
    Copyright (C) 2015 by Ruwen Hahn <palana@stunned.de>

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

#include "OBSSourceLabel.hpp"
#include "moc_OBSSourceLabel.cpp"

OBSSourceLabel::OBSSourceLabel(const obs_source_t *source, QWidget *parent, Qt::WindowFlags f)
	: QLabel(obs_source_get_name(source), parent, f),
	  renamedSignal(obs_source_get_signal_handler(source), "rename", &OBSSourceLabel::obsSourceRenamed, this),
	  removedSignal(obs_source_get_signal_handler(source), "remove", &OBSSourceLabel::obsSourceRemoved, this),
	  destroyedSignal(obs_source_get_signal_handler(source), "destroy", &OBSSourceLabel::obsSourceDestroyed, this)
{
}

void OBSSourceLabel::obsSourceRenamed(void *data, calldata_t *params)
{
	auto &label = *static_cast<OBSSourceLabel *>(data);

	const char *name = calldata_string(params, "new_name");
	label.setText(name);

	emit label.renamed(name);
}

void OBSSourceLabel::obsSourceRemoved(void *data, calldata_t *)
{
	auto &label = *static_cast<OBSSourceLabel *>(data);
	emit label.removed();
}

void OBSSourceLabel::obsSourceDestroyed(void *data, calldata_t *)
{
	auto &label = *static_cast<OBSSourceLabel *>(data);
	emit label.destroyed();

	label.destroyedSignal.Disconnect();
	label.removedSignal.Disconnect();
	label.renamedSignal.Disconnect();
}

void OBSSourceLabel::mousePressEvent(QMouseEvent *event)
{
	emit clicked();

	QLabel::mousePressEvent(event);
}
