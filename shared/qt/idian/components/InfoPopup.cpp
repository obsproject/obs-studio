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

#include <Idian/InfoPopup.hpp>
#include <Idian/PopupContents.hpp>

namespace idian {

InfoPopup::InfoPopup(QWidget *parent) : idian::AbstractPopup(parent), idian::Utils(this)
{
	guideWidget = new QFrame(this);
	guideLayout = new QVBoxLayout(guideWidget);
	guideLayout->setContentsMargins(0, 0, 0, 0);
	guideLayout->setSpacing(0);

	header = new QWidget(this);
	addClass(header, "popup-header");
	headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(0);

	title = new QLabel(this);
	title->setText("Info Popup");
	title->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	dismissButton = new QPushButton(this);
	addClass(dismissButton, "icon-close");
	addClass(dismissButton, "tooltip-close");

	headerLayout->addWidget(title);
	headerLayout->addWidget(dismissButton);

	info = new QLabel(this);
	addClass(info, "popup-text");
	info->setWordWrap(true);
	info->setTextFormat(Qt::MarkdownText);
	info->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	guideLayout->addWidget(header);
	guideLayout->addWidget(info);
	guideLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));

	setOrientation(Qt::Vertical);
	setWidget(guideWidget);

	connect(dismissButton, &QAbstractButton::clicked, this, &InfoPopup::dismiss);

	repolish(this);
	polishChildren(this);
}

InfoPopup::~InfoPopup() {}

void InfoPopup::setTitle(QString text)
{
	title->setText(text);
}

void InfoPopup::setInfo(QString text)
{
	info->setText(text);
}

void InfoPopup::dismiss()
{
	emit rejected();
	deleteLater();
}

} // namespace idian
