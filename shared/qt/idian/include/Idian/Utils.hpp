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

#include <QPointer>
#include <QRegularExpression>
#include <QStyle>
#include <QWidget>

class QAbstractButton;
class QLabel;

namespace idian {

// Helpers for OBS Idian widgets
class Utils : public QObject {
	Q_OBJECT

	static bool classNameIsValid(const QString &name)
	{
		static const QRegularExpression classRegex("^[a-zA-Z][a-zA-Z0-9_-]*$");
		const QRegularExpressionMatch match = classRegex.match(name);
		return match.hasMatch();
	}

	bool isPolishPending{false};
	std::vector<QPointer<QWidget>> widgetPolishQueue;

public:
	Utils();

	void addToPolishQueue(QWidget *widget);
	void removeFromPolishQueue(QWidget *widget);
	bool isQueuedForPolish(QWidget *widget) const;
	void processPolishQueue();

	// Forces an immediate polish of the widget and removes it from the queue.
	static void polishNow(QWidget *widget);

	// Queue all children widgets to repaint
	static void polishChildren(QWidget *widget);

	// Adds a style class to the widget. Queues it for a repolish only if it was not present already.
	static void addClass(QWidget *widget, const QString &classname);

	// Removes a style class from a widget. Queues it for a repolish only if it was present.
	static void removeClass(QWidget *widget, const QString &classname);

	// Calls addClass or removeClass for the given widget and classname based on toggle.
	static void toggleClass(QWidget *widget, const QString &classname, bool toggle);

	static void applyColorToIcon(QAbstractButton *button);
	static void applyColorToIcon(QLabel *label);

	static QPixmap recolorPixmap(const QPixmap &src, const QColor &color);

	static void applyStateStylingEventFilter(QWidget *widget);
};

} // namespace idian
