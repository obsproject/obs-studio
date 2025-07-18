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

#include "AlignmentSelector.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QStyleOptionButton>

#include <util/base.h>

AlignmentSelector::AlignmentSelector(QWidget *parent) : QWidget(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover);
}

QSize AlignmentSelector::sizeHint() const
{
	int base = fontMetrics().height() * 2;
	return QSize(base, base);
}

QSize AlignmentSelector::minimumSizeHint() const
{
	return QSize(16, 16);
}

Qt::Alignment AlignmentSelector::value() const
{
	return cellAlignment(selectedCell);
}

int AlignmentSelector::currentIndex() const
{
	return selectedCell;
}

void AlignmentSelector::setAlignment(Qt::Alignment value)
{
	alignment = value;
}

void AlignmentSelector::setCurrentIndex(int index)
{
	selectCell(index);
}

void AlignmentSelector::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	QStyle *style = this->style();

	int cellW = gridRect().width() / 3;
	int cellH = gridRect().height() / 3;

	for (int i = 0; i < 9; ++i) {
		QRect rect = cellRect(i);
		rect = rect.adjusted(0, 0, -1, -1);

		QStyleOptionFrame frameOpt;
		frameOpt.rect = rect;
		frameOpt.state = isEnabled() ? QStyle::State_Enabled : QStyle::State_None;
		frameOpt.lineWidth = 1;
		frameOpt.midLineWidth = 0;
		if (i == hoveredCell) {
			frameOpt.state |= QStyle::State_MouseOver;
		}
		if (i == selectedCell) {
			frameOpt.state |= QStyle::State_On;
		}
		if (i == focusedCell && hasFocus()) {
			frameOpt.state |= QStyle::State_HasFocus;
		}

		QStyleOptionButton radioOpt;
		radioOpt.state = isEnabled() ? QStyle::State_Enabled : QStyle::State_None;
		radioOpt.rect = rect.adjusted(cellW / 6, cellH / 6, -cellW / 6, -cellH / 6);
		if (i == hoveredCell) {
			radioOpt.state |= QStyle::State_MouseOver;
		}
		if (i == selectedCell) {
			radioOpt.state |= QStyle::State_On;
		}

		if (i == focusedCell && hasFocus()) {
			radioOpt.state |= QStyle::State_HasFocus;
		}
		style->drawPrimitive(QStyle::PE_IndicatorRadioButton, &radioOpt, &painter, this);

		style->drawPrimitive(QStyle::PE_Frame, &frameOpt, &painter, this);

		if (i == focusedCell && hasFocus()) {
			QStyleOptionFocusRect focusOpt;
			focusOpt.initFrom(this);
			focusOpt.rect = rect.adjusted(1, 1, -1, -1);
			focusOpt.state = isEnabled() ? QStyle::State_Enabled : QStyle::State_None;
			focusOpt.state |= QStyle::State_HasFocus;
			style->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOpt, &painter, this);
		}
	}
}

QRect AlignmentSelector::cellRect(int index) const
{
	int col = index % 3;
	int row = index / 3;

	QRect gridRect = this->gridRect();
	int cellW = gridRect.width() / 3;
	int cellH = gridRect.height() / 3;

	return QRect(col * cellW + gridRect.left(), row * cellH + gridRect.top(), cellW, cellH);
}

Qt::Alignment AlignmentSelector::cellAlignment(int index) const
{
	Qt::Alignment hAlign, vAlign;

	switch (index % 3) {
	case 0:
		hAlign = Qt::AlignLeft;
		break;
	case 1:
		hAlign = Qt::AlignHCenter;
		break;
	case 2:
		hAlign = Qt::AlignRight;
		break;
	}

	switch (index / 3) {
	case 0:
		vAlign = Qt::AlignTop;
		break;
	case 1:
		vAlign = Qt::AlignVCenter;
		break;
	case 2:
		vAlign = Qt::AlignBottom;
		break;
	}

	return hAlign | vAlign;
}

void AlignmentSelector::leaveEvent(QEvent *)
{
	hoveredCell = -1;
	update();
}

void AlignmentSelector::mouseMoveEvent(QMouseEvent *event)
{
	QRect grid = gridRect();
	int cellW = grid.width() / 3;
	int cellH = grid.height() / 3;

	QPoint pos = event->position().toPoint();
	if (!grid.contains(pos)) {
		hoveredCell = -1;
		return;
	}

	int col = (pos.x() - grid.left()) / cellW;
	int row = (pos.y() - grid.top()) / cellH;
	int cell = row * 3 + col;

	if (hoveredCell != cell) {
		hoveredCell = cell;
		update();
	}
}

void AlignmentSelector::mousePressEvent(QMouseEvent *event)
{
	QRect grid = gridRect();
	int cellW = grid.width() / 3;
	int cellH = grid.height() / 3;

	QPoint pos = event->position().toPoint();
	if (!grid.contains(pos)) {
		return;
	}

	int col = (pos.x() - grid.left()) / cellW;
	int row = (pos.y() - grid.top()) / cellH;
	int cell = row * 3 + col;

	selectCell(cell);
}

void AlignmentSelector::keyPressEvent(QKeyEvent *event)
{
	int moveX = 0, moveY = 0;
	switch (event->key()) {
	case Qt::Key_Left:
		moveX = -1;
		break;
	case Qt::Key_Right:
		moveX = 1;
		break;
	case Qt::Key_Up:
		moveY = -1;
		break;
	case Qt::Key_Down:
		moveY = 1;
		break;
	case Qt::Key_Space:
	case Qt::Key_Return:
	case Qt::Key_Enter:
		selectCell(focusedCell);
		return;
	default:
		QWidget::keyPressEvent(event);
		return;
	}

	moveFocusedCell(moveX, moveY);
}

QRect AlignmentSelector::gridRect() const
{
	int side = std::min(width(), height());
	int x = 0;
	int y = 0;

	if (alignment & Qt::AlignHCenter) {
		x = (width() - side) / 2;
	} else if (alignment & Qt::AlignRight) {
		x = width() - side;
	}

	if (alignment & Qt::AlignVCenter) {
		y = (height() - side) / 2;
	} else if (alignment & Qt::AlignBottom) {
		y = height() - side;
	}

	return QRect(x, y, side, side);
}

void AlignmentSelector::moveFocusedCell(int moveX, int moveY)
{
	int row = focusedCell / 3;
	int col = focusedCell % 3;

	row = std::clamp(row + moveY, 0, 2);
	col = std::clamp(col + moveX, 0, 2);

	int newCell = row * 3 + col;
	if (newCell != focusedCell) {
		focusedCell = newCell;
		update();
	}
}

void AlignmentSelector::selectCell(int cell)
{
	if (cell != selectedCell) {
		selectedCell = cell;
		emit valueChanged(cellAlignment(cell));
		emit currentIndexChanged(cell);
	}
	focusedCell = cell;
	update();
}

void AlignmentSelector::focusInEvent(QFocusEvent *)
{
	update();
}

void AlignmentSelector::focusOutEvent(QFocusEvent *)
{
	hoveredCell = -1;
	update();
}
