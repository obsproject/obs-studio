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

#include <QWidget>
#include <QStyle>
#include <QFocusEvent>
#include <QRegularExpression>

/*
 * Helpers for OBS Idian widgets
 */

class OBSWidgetUtils {

private:
	QRegularExpression classRegex =
		QRegularExpression("^[a-zA-Z][a-zA-Z0-9]*$");

	bool classNameIsValid(const QString &name)
	{
		QRegularExpressionMatch match = classRegex.match(name);
		return match.hasMatch();
	}

public:
	QWidget *widget = nullptr;

	OBSWidgetUtils(QWidget *w) { widget = w; }

	/*
	 * Set a custom property whenever the widget has
	 * keyboard focus specifically
	 */
	void showKeyFocused(QFocusEvent *e)
	{
		if (e->reason() != Qt::MouseFocusReason &&
		    e->reason() != Qt::PopupFocusReason) {
			addClass("keyFocus");
		} else {
			removeClass("keyFocus");
		}
	};
	void hideKeyFocused(QFocusEvent *e)
	{
		if (e->reason() != Qt::PopupFocusReason) {
			removeClass("keyFocus");
		}
	};

	/*
	* Force all children widgets to repaint
	*/
	void polishChildren()
	{
		QStyle *const style = widget->style();
		for (QWidget *child : widget->findChildren<QWidget *>()) {
			style->unpolish(child);
			style->polish(child);
		}
	}

	void addClass(QWidget *widget, const QString &name)
	{
		if (!classNameIsValid(name)) {
			return;
		}

		QVariant current = widget->property("class");

		QStringList classList = current.toString().split(" ");
		if (classList.contains(name)) {
			return;
		}

		classList.removeDuplicates();
		classList.append(name);

		widget->setProperty("class", classList.join(" "));

		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
	}

	void addClass(const QString &name) { addClass(widget, name); }

	void removeClass(QWidget *widget, const QString &name)
	{
		if (!classNameIsValid(name)) {
			return;
		}

		QVariant current = widget->property("class");
		if (current.isNull()) {
			return;
		}

		QStringList classList = current.toString().split(" ");
		if (!classList.contains(name, Qt::CaseSensitive)) {
			return;
		}

		classList.removeDuplicates();
		classList.removeAll(name);

		widget->setProperty("class", classList.join(" "));

		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
	}

	void removeClass(const QString &name) { removeClass(widget, name); }

	void toggleClass(QWidget *widget, const QString &name, bool toggle)
	{
		if (toggle) {
			addClass(widget, name);
		} else {
			removeClass(widget, name);
		}
	}

	void toggleClass(const QString &name, bool toggle)
	{
		toggleClass(widget, name, toggle);
	}
};
