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

#pragma once

#include <components/SegmentButton.hpp>
#include <components/SegmentButtonGroup.hpp>

#include <QFrame>
#include <QBoxLayout>

class SegmentButtonFrame : public QFrame {
	Q_OBJECT

	Q_PROPERTY(int buttonSpacing READ getButtonSpacing WRITE setButtonSpacing DESIGNABLE true)

public:
	SegmentButtonFrame(QWidget *parent = nullptr);

	QHBoxLayout *layout() const { return layout_; }
	SegmentButtonGroup *group() const { return group_; }

	void addButton(SegmentButton *button, int id);
	void removeButton(SegmentButton *button);

	SegmentButton *button(int id) const;
	QList<SegmentButton *> buttons() const;

	SegmentButton *checkedButton() const;
	int checkedId() const;

private:
	SegmentButtonGroup *group_;
	QHBoxLayout *layout_;

	int buttonSpacing;
	int getButtonSpacing() { return buttonSpacing; }
	void setButtonSpacing(int spacing)
	{
		buttonSpacing = spacing;
		layout()->setSpacing(buttonSpacing);
	}

	void updateStyleClasses();
};
