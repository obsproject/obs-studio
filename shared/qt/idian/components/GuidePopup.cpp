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

#include <Idian/GuidePopup.hpp>
#include <Idian/PopupContents.hpp>

namespace idian {

GuidePopup::GuidePopup(QWidget *parent) : idian::AbstractPopup(parent), idian::Utils(this)
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
	title->setText("Guide Popup");
	title->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	headerLayout->addWidget(title);

	info = new QLabel(this);
	addClass(info, "popup-text");
	info->setWordWrap(true);
	info->setTextFormat(Qt::MarkdownText);
	info->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	footer = new QFrame(this);
	footerLayout = new QHBoxLayout(footer);
	addClass(footer, "popup-footer");
	footerLayout->setContentsMargins(0, 0, 0, 0);
	footerLayout->setSpacing(0);

	footerSteps = new QLabel(this);

	footerNext = new QPushButton("Next", footer);
	addClass(footerNext, "button-primary");

	footerLayout->addWidget(footerSteps);
	footerLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
	footerLayout->addWidget(footerNext);

	guideLayout->addWidget(header);
	guideLayout->addWidget(info);
	guideLayout->addWidget(footer);
	guideLayout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));

	setOrientation(Qt::Vertical);
	setWidget(guideWidget);

	repolish(this);
	polishChildren(this);
}

GuidePopup::~GuidePopup() {}

void GuidePopup::setTitle(QString text)
{
	title->setText(text);
}

void GuidePopup::setInfo(QString text)
{
	info->setText(text);
}

void GuidePopup::setStep(int step)
{
	this->step = step;

	updateSteps();
}

void GuidePopup::setTotalSteps(int total)
{

	this->totalSteps = total;

	multipleSteps = (total > 1);
	footer->setVisible(multipleSteps);

	updateSteps();
}

void GuidePopup::updateSteps()
{
	footerSteps->setText(QString("Step %1 / %2").arg(step).arg(totalSteps));
}

void GuidePopup::next()
{
	emit accepted();
	deleteLater();
}

} // namespace idian
