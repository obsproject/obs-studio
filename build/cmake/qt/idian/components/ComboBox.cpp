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

#include <Idian/ComboBox.hpp>

#include <Idian/Row.hpp>

#include <QTimer>

#include <Idian/moc_ComboBox.cpp>

using idian::ComboBox;

ComboBox::ComboBox(QWidget *parent) : QComboBox(parent), Utils(this) {}

void ComboBox::showPopup()
{
	if (allowOpeningPopup) {
		QComboBox::showPopup();
	}
}

void ComboBox::hidePopup()
{
	// It would be nice to find a better way to do this.
	//
	// When the dropdown is closed, block attempts to open it
	// again for a short time. This is so clicking a GenericRow
	// with the dropdown open doesn't immediately close and re-open it.
	// I have tried all sorts of things involving handling mouse events
	// and event filters on both GenericRow and ComboBox.
	//
	// All my efforts have failed so we get this instead.
	allowOpeningPopup = false;
	QTimer::singleShot(120, this, [this]() { allowOpeningPopup = true; });

	QComboBox::hidePopup();
}

void ComboBox::mousePressEvent(QMouseEvent *event)
{
	QComboBox::mousePressEvent(event);
}

void ComboBox::togglePopup()
{
	if (view()->isVisible()) {
		ComboBox::hidePopup();
	} else {
		ComboBox::showPopup();
	}
}
