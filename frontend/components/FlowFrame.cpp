/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <components/FlowFrame.hpp>

#include <QKeyEvent>
#include <QScrollArea>

FlowFrame::FlowFrame(QWidget *parent) : QFrame(parent)
{
	layout = new FlowLayout(this);
	setLayout(layout);
}

void FlowFrame::keyPressEvent(QKeyEvent *event)
{
	QWidget *focused = focusWidget();
	if (!focused) {
		return;
	}

	int index = -1;
	for (int i = 0; i < layout->count(); ++i) {
		if (!layout->itemAt(i)->widget()) {
			continue;
		}

		auto focusProxy = layout->itemAt(i)->widget()->focusProxy();
		if (layout->itemAt(i)->widget() == focused || focusProxy == focused) {
			if (focusProxy == focused) {
				focused = layout->itemAt(i)->widget();
			}

			index = i;
			break;
		}
	}

	if (index == -1) {
		return;
	}

	const QRect focusedRect = focused->geometry();
	QWidget *nextFocus = nullptr;

	switch (event->key()) {
	case Qt::Key_Right:
	case Qt::Key_Down:
	case Qt::Key_Left:
	case Qt::Key_Up: {
		/* Find next widget in the given direction */
		int bestDistance = INT_MAX;
		for (int i = 0; i < layout->count(); ++i) {
			if (i == index) {
				continue;
			}

			QWidget *widget = layout->itemAt(i)->widget();
			const QRect rect = widget->geometry();

			bool isCandidate = false;
			int distance = INT_MAX;

			switch (event->key()) {
			case Qt::Key_Right:
				if (rect.left() > focusedRect.right()) {
					distance = (rect.left() - focusedRect.right()) +
						   qAbs(rect.center().y() - focusedRect.center().y());
					isCandidate = true;
				}
				break;
			case Qt::Key_Left:
				if (rect.right() < focusedRect.left()) {
					distance = (focusedRect.left() - rect.right()) +
						   qAbs(rect.center().y() - focusedRect.center().y());
					isCandidate = true;
				}
				break;
			case Qt::Key_Down:
				if (rect.top() > focusedRect.bottom()) {
					distance = (rect.top() - focusedRect.bottom()) +
						   qAbs(rect.center().x() - focusedRect.center().x());
					isCandidate = true;
				}
				break;
			case Qt::Key_Up:
				if (rect.bottom() < focusedRect.top()) {
					distance = (focusedRect.top() - rect.bottom()) +
						   qAbs(rect.center().x() - focusedRect.center().x());
					isCandidate = true;
				}
				break;
			}

			if (isCandidate && distance < bestDistance) {
				bestDistance = distance;
				nextFocus = widget;
			}
		}
		break;
	}
	default:
		QWidget::keyPressEvent(event);
		return;
	}

	if (nextFocus) {
		nextFocus->setFocus();

		QWidget *scrollParent = nextFocus->parentWidget();
		while (scrollParent) {
			QScrollArea *scrollArea = qobject_cast<QScrollArea *>(scrollParent);
			if (scrollArea) {
				scrollArea->ensureWidgetVisible(nextFocus, 20, 20);
				break;
			}
			scrollParent = scrollParent->parentWidget();
		}
	}
}
