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

#include <QStyle>

#include "obs-propertieslist.hpp"
#include "obs-actionrow.hpp"

OBSPropertiesList::OBSPropertiesList(QWidget *parent) : QFrame(parent)
{
	layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	rowsList = QList<OBSActionBaseClass *>();

	setLayout(layout);
}

/* Note: This function takes ownership of the added widget
 * and it may be deleted when the properties list is destroyed
 * or the clear() method is called! */
void OBSPropertiesList::addRow(OBSActionBaseClass *ar)
{
	// Add custom spacer once more than one element exists
	if (layout->count() > 0)
		layout->addWidget(new OBSPropertiesListSpacer(this));

	// Custom properties to work around :first and :last not existing.
	if (!first) {
		ar->setProperty("first", true);
		first = ar;
	}

	// Remove last property from existing last item
	if (last)
		last->setProperty("last", QVariant());

	// Most recently added item is also always last
	ar->setProperty("last", true);
	last = ar;

	ar->setParent(this);
	rowsList.append(ar);
	layout->addWidget(ar);
	adjustSize();
}

void OBSPropertiesList::clear()
{
	rowsList.clear();
	first = nullptr;
	last = nullptr;
	QLayoutItem *item = layout->takeAt(0);

	while (item) {
		if (item->widget())
			item->widget()->deleteLater();
		delete item;

		item = layout->takeAt(0);
	}

	adjustSize();
}
