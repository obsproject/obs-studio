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

#include <Idian/RowList.hpp>

#include <QStyle>

#include <Idian/moc_RowList.cpp>

using idian::RowList;

RowList::RowList(QWidget *parent) : QFrame(parent)
{
	layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

	setLayout(layout);

	rowLayout = new QVBoxLayout();

	layout->addLayout(rowLayout);
}

void idian::RowList::addHeader(QWidget *widget)
{
	layout->insertWidget(layout->indexOf(rowLayout) - 1, widget);
}

// Note: This function takes ownership of the added widget
// and it may be deleted when the properties list is destroyed
// or the clear() method is called!
void RowList::addRow(QWidget *widget)
{
	// Add custom spacer when more than one row exists
	if (rowLayout->count() > 0) {
		rowLayout->addWidget(new RowListSpacer(this));
	}

	// Custom properties to work around :first and :last not existing.
	if (!first) {
		Utils::addClass(widget, "first");
		first = widget;
	}

	// Remove last property from existing last item
	if (last) {
		Utils::removeClass(last, "last");
	}

	// Most recently added item is also always last
	Utils::addClass(widget, "last");
	last = widget;

	rowLayout->addWidget(widget);
	adjustSize();
}

void RowList::clear()
{
	first = nullptr;
	last = nullptr;
	QLayoutItem *item = rowLayout->takeAt(0);

	while (item) {
		if (item->widget()) {
			item->widget()->deleteLater();
		}
		delete item;

		item = rowLayout->takeAt(0);
	}

	adjustSize();
}
