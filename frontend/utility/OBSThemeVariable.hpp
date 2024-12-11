/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

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

#include <QString>
#include <QVariant>

struct OBSThemeVariable {
	enum VariableType {
		Color,  /* RGB color value*/
		Size,   /* Number with suffix denoting size (e.g. px, pt, em) */
		Number, /* Number without suffix */
		String, /* Raw string (e.g. color name, border style, etc.) */
		Alias,  /* Points at another variable, value will be the key */
		Calc,   /* Simple calculation with two operands */
	};

	/* Whether the variable should be editable in the UI */
	bool editable = false;
	/* Used for VariableType::Size only */
	QString suffix;

	VariableType type;
	QString name;
	QVariant value;
	QVariant userValue; /* If overwritten by user, use this value instead */
};
