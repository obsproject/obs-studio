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

#include <QFocusEvent>
#include <QRegularExpression>
#include <QStyle>
#include <QWidget>

namespace idian {

// Helpers for OBS Idian widgets
class Utils {

	static bool classNameIsValid(const QString &name)
	{
		static const QRegularExpression classRegex("^[a-zA-Z][a-zA-Z0-9_-]*$");
		const QRegularExpressionMatch match = classRegex.match(name);
		return match.hasMatch();
	}

public:
	QWidget *parent = nullptr;

	Utils(QWidget *w) { parent = w; }

	// Set a custom property whenever the widget has keyboard focus specifically
	void showKeyFocused(QFocusEvent *e)
	{
		if (e->reason() != Qt::MouseFocusReason && e->reason() != Qt::PopupFocusReason) {
			addClass("keyFocus");
		} else {
			removeClass("keyFocus");
		}
	}

	void hideKeyFocused(QFocusEvent *e)
	{
		if (e->reason() != Qt::PopupFocusReason) {
			removeClass("keyFocus");
		}
	}

	// Force all children widgets to repaint
	void polishChildren() { polishChildren(parent); }

	static void polishChildren(QWidget *widget)
	{
		for (QWidget *child : widget->findChildren<QWidget *>()) {
			repolish(child);
		}
	}

	void repolish() { repolish(parent); }

	static void repolish(QWidget *widget)
	{
		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
	}

	// Adds a style class to the widget
	void addClass(const QString &classname) { addClass(parent, classname); }

	static void addClass(QWidget *widget, const QString &classname)
	{
		if (!classNameIsValid(classname)) {
			return;
		}

		QVariant current = widget->property("class");

		QStringList classList = current.toString().split(" ");
		if (classList.contains(classname)) {
			return;
		}

		classList.removeDuplicates();
		classList.append(classname);

		widget->setProperty("class", classList.join(" "));

		repolish(widget);
	}

	// Removes a style class from a widget
	void removeClass(const QString &classname) { removeClass(parent, classname); }

	static void removeClass(QWidget *widget, const QString &classname)
	{
		if (!classNameIsValid(classname)) {
			return;
		}

		QVariant current = widget->property("class");
		if (current.isNull()) {
			return;
		}

		QStringList classList = current.toString().split(" ");
		if (!classList.contains(classname, Qt::CaseSensitive)) {
			return;
		}

		classList.removeDuplicates();
		classList.removeAll(classname);

		widget->setProperty("class", classList.join(" "));

		repolish(widget);
	}

	// Forces the addition or removal of a style class from a widget
	void toggleClass(const QString &classname, bool toggle) { toggleClass(parent, classname, toggle); }

	static void toggleClass(QWidget *widget, const QString &classname, bool toggle)
	{
		if (toggle) {
			addClass(widget, classname);
		} else {
			removeClass(widget, classname);
		}
	}
};

} // namespace idian
