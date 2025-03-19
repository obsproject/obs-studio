/******************************************************************************
    Copyright (C) 2024 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "OBSActionRow.hpp"
#include "OBSComboBox.hpp"
#include <QTimer>
#include <util/base.h>

#define UNUSED_PARAMETER(param) (void)param

OBSComboBox::OBSComboBox(QWidget *parent) : QComboBox(parent), OBSIdianUtils(this) {}

void OBSComboBox::showPopup()
{
	if (allowOpeningPopup) {
		allowOpeningPopup = false;
		QComboBox::showPopup();
	}
}

void OBSComboBox::hidePopup()
{
	/* ToDo: Find a better way to do this
	 * When the dropdown is closed, block attempts to open it
	 * again for a short time. This is so clicking an OBSActionRow
	 * with the dropdown open doesn't immediately close and re-open it.
	 * 
	 * I have tried all sorts of things involving handling mouse events
	 * and event filters on both OBSActionRow and OBSComboBox.
	 * All my efforts have failed so we get this instead.
	 */
	allowOpeningPopup = false;
	QTimer::singleShot(120, this, [=]() { allowOpeningPopup = true; });

	QComboBox::hidePopup();
}

void OBSComboBox::mousePressEvent(QMouseEvent *event)
{
	blog(LOG_INFO, "OBSComboBox::mousePressEvent");
	QComboBox::mousePressEvent(event);
}

void OBSComboBox::togglePopup()
{
	if (view()->isVisible()) {
		OBSComboBox::hidePopup();
	} else {
		OBSComboBox::showPopup();
	}
}
