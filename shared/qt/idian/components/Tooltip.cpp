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

#include <Idian/PopupContents.hpp>
#include <Idian/Tooltip.hpp>

namespace idian {

Tooltip::Tooltip(QWidget *target) : idian::AbstractPopup(target), idian::Utils(this)
{
	label = new QLabel(this);

	setAnchorWidget(target, idian::Anchor::TopCenter);
	setAnchorFrom(idian::Anchor::BottomCenter);
	setOrientation(Qt::Vertical);

	innerFrame->setShowArrow(false);

	setWindowFlag(Qt::WindowTransparentForInput, true);
	setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);
	setAttribute(Qt::WA_TransparentForMouseEvents, true);

	setWidget(label);
	label->setAlignment(Qt::AlignVCenter);

	hide();
}

Tooltip::~Tooltip() {}

void Tooltip::setText(QString text)
{
	if (label->text() == text) {
		return;
	}

	label->setText(text);

	// Handle very long tooltips with word wrapping
	if (label->sizeHint().width() > TOOLTIP_WRAP_WIDTH) {
		label->setMinimumWidth(TOOLTIP_WRAP_WIDTH);
		label->setWordWrap(true);
		setAnchorWidget(anchorWidget, idian::Anchor::TopLeft);
		setAnchorFrom(idian::Anchor::BottomLeft);
	} else if (label->wordWrap()) {
		label->setMinimumWidth(0);
		label->setWordWrap(false);
		setAnchorWidget(anchorWidget, idian::Anchor::TopCenter);
		setAnchorFrom(idian::Anchor::BottomCenter);
	}

	queuePositionUpdate();
}

void Tooltip::show()
{
	if (isEnabled()) {
		AbstractPopup::show();
	} else {
		AbstractPopup::hide();
	}
}

} // namespace idian
