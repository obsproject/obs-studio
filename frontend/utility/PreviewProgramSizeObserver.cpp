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

#include "PreviewProgramSizeObserver.hpp"

#include <QEvent>
#include <QLayout>
#include <QResizeEvent>
#include <QTimer>

#include <util/base.h>

PreviewProgramSizeObserver::PreviewProgramSizeObserver(QWidget *widgetLeft, QWidget *widgetRight, QObject *parent)
	: QObject(parent),
	  left(widgetLeft),
	  right(widgetRight)
{
	if (!left || !right) {
		return;
	}

	std::pair<QWidget *, QWidget *> siblingParents = findSiblingParents(left, right);
	leftContainer = siblingParents.first;
	rightContainer = siblingParents.second;

	leftOriginalMaxSize = leftContainer->maximumSize();
	rightOriginalMaxSize = rightContainer->maximumSize();

	QWidget *sharedParent = leftContainer->parentWidget();

	if (!sharedParent) {
		return;
	}
	ancestorContainer = sharedParent;

	QLayout *ancestorLayout = ancestorContainer->layout();
	if (QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(ancestorLayout)) {
		setOrientation((boxLayout->direction() == QBoxLayout::LeftToRight) ? Qt::Horizontal : Qt::Vertical);
	}

	leftTargetSize = left->width();
	rightTargetSize = right->width();

	ancestorContainer->installEventFilter(this);

	connect(ancestorContainer, &QWidget::destroyed, this, &QWidget::deleteLater);
	connect(left, &QWidget::destroyed, this, &QWidget::deleteLater);
	connect(right, &QWidget::destroyed, this, &QWidget::deleteLater);
	connect(leftContainer, &QWidget::destroyed, this, &QWidget::deleteLater);
	connect(rightContainer, &QWidget::destroyed, this, &QWidget::deleteLater);
}

PreviewProgramSizeObserver::~PreviewProgramSizeObserver()
{
	if (ancestorContainer) {
		ancestorContainer->removeEventFilter(this);
	}
	if (leftContainer) {
		leftContainer->setMaximumSize(leftOriginalMaxSize);
	}

	if (rightContainer) {
		rightContainer->setMaximumSize(rightOriginalMaxSize);
	}
}

void PreviewProgramSizeObserver::setOrientation(Qt::Orientation orientation_)
{
	orientation = orientation_;
}

std::pair<QWidget *, QWidget *> PreviewProgramSizeObserver::findSiblingParents(QWidget *a, QWidget *b)
{
	// Search through ancestors of two widgets to find the topmost pair that are siblings
	QWidget *ancestor1 = a;
	QWidget *ancestor2 = b;

	while (ancestor1 && ancestor2) {
		QWidget *parent1 = ancestor1->parentWidget();
		QWidget *parent2 = ancestor2->parentWidget();

		if (!parent1 || !parent2) {
			break;
		}

		// Found sibling containers
		if (parent1 == parent2) {
			return {ancestor1, ancestor2};
		}

		if (parent1->isAncestorOf(parent2)) {
			ancestor2 = parent2;
		} else if (parent2->isAncestorOf(parent1)) {
			ancestor1 = parent1;
		} else {
			ancestor1 = parent1;
			ancestor2 = parent2;
		}
	}

	return {a, b};
}

void PreviewProgramSizeObserver::syncContainerSizes(int containerSizeDelta)
{
	auto setMax = (orientation == Qt::Horizontal) ? [](QWidget* widget, int value) {
		widget->setMaximumWidth(value);
		} : [](QWidget* widget, int value) {
		widget->setMaximumHeight(value);
		};

	QLayout *ancestorLayout = ancestorContainer->layout();
	if (QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(ancestorLayout)) {
		setOrientation((boxLayout->direction() == QBoxLayout::LeftToRight) ? Qt::Horizontal : Qt::Vertical);
	}

	if (orientation == Qt::Horizontal) {
		leftContainer->setMaximumHeight(leftOriginalMaxSize.height());
		rightContainer->setMaximumHeight(rightOriginalMaxSize.height());
	} else {
		leftContainer->setMaximumWidth(leftOriginalMaxSize.width());
		rightContainer->setMaximumWidth(rightOriginalMaxSize.width());
	}

	int leftInner = (orientation == Qt::Horizontal) ? left->width() : left->height();
	int rightInner = (orientation == Qt::Horizontal) ? right->width() : right->height();

	int leftOuter = (orientation == Qt::Horizontal) ? leftContainer->width() : leftContainer->height();
	int rightOuter = (orientation == Qt::Horizontal) ? rightContainer->width() : rightContainer->height();

	int totalOuter = leftOuter + rightOuter;
	if (containerSizeDelta >= 0) {
		totalOuter += containerSizeDelta;
	}

	int leftOffset = leftOuter - leftInner;
	int rightOffset = rightOuter - rightInner;

	int targetInner = (totalOuter - leftOffset - rightOffset) / 2;

	leftTargetSize = targetInner + leftOffset;
	rightTargetSize = targetInner + rightOffset;

	if (containerSizeDelta >= 0) {
		setMax(leftContainer, leftTargetSize);
		setMax(rightContainer, rightTargetSize);
	} else {
		// Container shrunk, only set max size on larger widget
		if (leftInner > rightInner) {
			setMax(leftContainer, leftTargetSize);
			setMax(rightContainer, QWIDGETSIZE_MAX);
		} else {
			setMax(leftContainer, QWIDGETSIZE_MAX);
			setMax(rightContainer, rightTargetSize);
		}

		// Force a second recalculation
		QTimer::singleShot(1, this, [&, setMax]() {
			if (updating) {
				return;
			}

			updating = true;

			setMax(leftContainer, leftTargetSize);
			setMax(rightContainer, rightTargetSize);

			updating = false;
		});
	}
}

bool PreviewProgramSizeObserver::eventFilter(QObject *target, QEvent *event)
{
	if (event->type() != QEvent::Resize && event->type() != QEvent::LayoutRequest) {
		return QObject::eventFilter(target, event);
	}

	if (!left || !right || !leftContainer || !rightContainer) {
		deleteLater();
		return QObject::eventFilter(target, event);
	}

	if (updating == true) {
		return QObject::eventFilter(target, event);
	}

	updating = true;

	if (event->type() == QEvent::LayoutRequest) {
		syncContainerSizes(0);
	} else if (event->type() == QEvent::Resize) {
		QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
		int newSize = (orientation == Qt::Horizontal) ? resizeEvent->size().width()
							      : resizeEvent->size().height();
		int oldSize = (orientation == Qt::Horizontal) ? resizeEvent->oldSize().width()
							      : resizeEvent->oldSize().height();

		if (newSize - oldSize != 0) {
			syncContainerSizes(newSize - oldSize);
		}
	}

	updating = false;

	return QObject::eventFilter(target, event);
}
