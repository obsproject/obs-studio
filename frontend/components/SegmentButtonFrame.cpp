/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "SegmentButtonFrame.hpp"

#include <Idian/Utils.hpp>

#include <QList>

SegmentButtonFrame::SegmentButtonFrame(QWidget *parent) : QFrame(parent)
{
	setFrameShape(QFrame::NoFrame);
	setFrameStyle(QFrame::Plain);
	setLineWidth(0);

	setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	layout_ = new QHBoxLayout(this);
	setLayout(layout_);
	layout()->setContentsMargins(0, 0, 0, 0);

	group_ = new SegmentButtonGroup(this);
	group()->setExclusive(true);
}

void SegmentButtonFrame::addButton(SegmentButton *button, int id)
{
	layout()->addWidget(button);
	group()->addButton(button, id);

	connect(button, &QAbstractButton::toggled, button, [button](bool checked) {
		if (checked) {
			button->raise();
		}
	});

	updateStyleClasses();
}

void SegmentButtonFrame::removeButton(SegmentButton *button)
{
	int buttonIndex = layout()->indexOf(button);
	if (buttonIndex == -1) {
		return;
	}

	layout()->removeWidget(button);
	group()->removeButton(button);

	updateStyleClasses();
}

SegmentButton *SegmentButtonFrame::button(int id) const
{
	return group()->button(id);
}

QList<SegmentButton *> SegmentButtonFrame::buttons() const
{
	return group()->buttons();
}

SegmentButton *SegmentButtonFrame::checkedButton() const
{
	return group()->checkedButton();
}

int SegmentButtonFrame::checkedId() const
{
	return group()->checkedId();
}

void SegmentButtonFrame::updateStyleClasses()
{

	const QHBoxLayout *layout = this->layout();

	bool foundFirstWidget = false;

	QWidget *currentWidget;

	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem *item = layout->itemAt(i);
		currentWidget = item->widget();

		if (!currentWidget) {
			continue;
		}

		if (!foundFirstWidget) {
			foundFirstWidget = true;
			idian::Utils::toggleClass(currentWidget, "first", true);
		} else {
			idian::Utils::toggleClass(currentWidget, "first", false);
		}

		idian::Utils::toggleClass(currentWidget, "last", false);
	}

	idian::Utils::toggleClass(currentWidget, "last", true);
}
