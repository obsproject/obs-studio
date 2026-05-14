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

#include <Idian/RowInfo.hpp>
#include <Idian/Utils.hpp>

#include <QLabel>
#include <QVBoxLayout>

namespace idian {
RowInfo::RowInfo(QWidget *parent) : QWidget(parent)
{
	widgetUtils = new idian::Utils(this);

	layout_ = new QVBoxLayout();
	layout_->setSpacing(0);
	layout_->setContentsMargins(0, 0, 0, 0);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	setLayout(layout_);

	nameLabel = new QLabel();
	nameLabel->setVisible(false);
	widgetUtils->addClass(nameLabel, "title");

	descriptionLabel = new QLabel();
	descriptionLabel->setVisible(false);
	widgetUtils->addClass(descriptionLabel, "description");

	layout_->addWidget(nameLabel);
	layout_->addWidget(descriptionLabel);
}

RowInfo::RowInfo(QWidget *parent, QString title) : RowInfo(parent)
{
	setTitle(title);
}

RowInfo::RowInfo(QWidget *parent, QString title, QString description) : RowInfo(parent)
{
	setTitle(title);
	setDescription(description);
}

void RowInfo::setTitle(const QString &name)
{
	if (name.isEmpty()) {
		showTitle(false);
		return;
	}

	nameLabel->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void RowInfo::setDescription(const QString &description)
{
	if (description.isEmpty()) {
		showDescription(false);
		return;
	}

	descriptionLabel->setText(description);
	setAccessibleDescription(description);
	showDescription(true);
}

void RowInfo::showTitle(bool visible)
{
	nameLabel->setVisible(visible);
}

void RowInfo::showDescription(bool visible)
{
	descriptionLabel->setVisible(visible);
}
} // namespace idian
