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

#pragma once

#include <QComboBox>
#include <QAbstractItemView>

#include "OBSIdianWidget.hpp"

class OBSComboBox : public QComboBox, public OBSIdianUtils {
	Q_OBJECT

public:
	OBSComboBox(QWidget *parent = nullptr);

public Q_SLOTS:
	void togglePopup();

private:
	bool allowOpeningPopup = true;

protected:
	void showPopup() override;
	void hidePopup() override;

	void mousePressEvent(QMouseEvent *event) override;

	void focusInEvent(QFocusEvent *e) override
	{
		OBSIdianUtils::showKeyFocused(e);
		QComboBox::focusInEvent(e);
	}

	void focusOutEvent(QFocusEvent *e) override
	{
		OBSIdianUtils::hideKeyFocused(e);
		QComboBox::focusOutEvent(e);
	}
};
