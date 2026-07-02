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

#include "SlidingStackedWidget.hpp"

#include <QTimer>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>

#include "moc_SlidingStackedWidget.cpp"

SlidingStackedWidget::SlidingStackedWidget(QWidget *parent) : QStackedWidget(parent) {}

void SlidingStackedWidget::setDuration(int duration)
{
	duration_ = duration;
}

int SlidingStackedWidget::duration()
{
	// TODO: Add Reduced Motion accessibility option in the future and return 0 here if it's set
	return duration_;
}

void SlidingStackedWidget::setEasingCurve(enum QEasingCurve::Type easingCurve)
{
	easingCurve_ = easingCurve;
}

void SlidingStackedWidget::setVertical(bool vertical)
{
	vertical_ = vertical;
}

void SlidingStackedWidget::setWrap(bool wrap)
{
	wrap_ = wrap;
}

bool SlidingStackedWidget::advanceNext()
{
	int current = currentIndex();

	if (wrap_ || (current < count() - 1)) {
		advanceToIndex(current + 1);
	} else {
		return false;
	}

	return true;
}

bool SlidingStackedWidget::advancePrevious()
{
	int current = currentIndex();

	if (wrap_ || (current > 0)) {
		advanceToIndex(current - 1);
	} else {
		return false;
	}

	return true;
}

void SlidingStackedWidget::setNextPageInstantly()
{
	nextPageInstant = true;
}

void SlidingStackedWidget::setCurrentIndex(int idx)
{
	advanceToIndex(idx);
}

void SlidingStackedWidget::setCurrentWidget(QWidget *widget)
{
	advanceToWidget(widget);
}

void SlidingStackedWidget::advanceToIndex(int idx, Direction direction)
{
	if (idx > count() - 1) {
		direction = vertical_ ? Direction::TopToBottom : Direction::RightToLeft;
		idx = (idx) % count();
	} else if (idx < 0) {
		direction = vertical_ ? Direction::BottomToTop : Direction::LeftToRight;
		idx = (idx + count()) % count();
	}

	advanceToWidget(widget(idx), direction);
}

void SlidingStackedWidget::advanceToWidget(QWidget *newWidget, Direction direction)
{
	int fromIndex = currentIndex();
	int nextIndex = indexOf(newWidget);

	if (isAnimating) {
		// Avoid interrupting animation if trying to advance from and to the same indexes.
		if (fromIndex == activeIndex_ && nextIndex == nextIndex_) {
			return;
		}

		// Skip rest of animation.
		animGroup->setCurrentTime(animGroup->duration());
		animGroup->stop();

		// Allow event loop to tick before re-initiating.
		QTimer::singleShot(0, this, [this, newWidget, direction]() { advanceToWidget(newWidget, direction); });

		return;
	}

	isAnimating = true;
	int animDuration = nextPageInstant ? 0 : duration();

	Direction directionHint;

	if (fromIndex == nextIndex) {
		isAnimating = false;
		return;
	} else if (fromIndex < nextIndex) {
		directionHint = vertical_ ? Direction::BottomToTop : Direction::RightToLeft;
	} else {
		directionHint = vertical_ ? Direction::TopToBottom : Direction::LeftToRight;
	}

	if (direction == Direction::Automatic) {
		direction = directionHint;
	}

	int offsetX = frameRect().width();
	int offsetY = frameRect().height();

	widget(nextIndex)->setGeometry(0, 0, offsetX, offsetY);
	if (direction == Direction::BottomToTop) {
		offsetX = 0;
		offsetY = -offsetY;
	} else if (direction == Direction::TopToBottom) {
		offsetX = 0;
	} else if (direction == Direction::RightToLeft) {
		offsetX = -offsetX;
		offsetY = 0;
	} else if (direction == Direction::LeftToRight) {
		offsetY = 0;
	}

	QPoint nextPos = widget(nextIndex)->pos();
	QPoint currentPos = widget(fromIndex)->pos();
	activePos_ = currentPos;
	widget(nextIndex)->move(nextPos.x() - offsetX, nextPos.y() - offsetY);

	widget(nextIndex)->show();
	widget(nextIndex)->raise();

	QPropertyAnimation *currentPosAnimation = new QPropertyAnimation(widget(fromIndex), "pos");
	currentPosAnimation->setDuration(animDuration);
	currentPosAnimation->setEasingCurve(easingCurve_);
	currentPosAnimation->setStartValue(QPoint(currentPos.x(), currentPos.y()));
	currentPosAnimation->setEndValue(QPoint(currentPos.x() + offsetX, currentPos.y() + offsetY));

	QGraphicsOpacityEffect *currentOpacityEffect = new QGraphicsOpacityEffect();
	widget(fromIndex)->setGraphicsEffect(currentOpacityEffect);

	QPropertyAnimation *currentOpacityAnimation = new QPropertyAnimation(currentOpacityEffect, "opacity");
	currentOpacityAnimation->setDuration(animDuration * 0.5);
	currentOpacityAnimation->setStartValue(1);
	currentOpacityAnimation->setEndValue(0);

	QGraphicsOpacityEffect *nextOpacityEffect = new QGraphicsOpacityEffect();
	nextOpacityEffect->setOpacity(0);
	widget(nextIndex)->setGraphicsEffect(nextOpacityEffect);

	QPropertyAnimation *nextOpacityAnimation = new QPropertyAnimation(nextOpacityEffect, "opacity");
	nextOpacityAnimation->setDuration(animDuration * 0.5);
	nextOpacityAnimation->setStartValue(0);
	nextOpacityAnimation->setEndValue(1);

	QPropertyAnimation *nextPosAnimation = new QPropertyAnimation(widget(nextIndex), "pos");
	nextPosAnimation->setDuration(animDuration);
	nextPosAnimation->setEasingCurve(easingCurve_);
	nextPosAnimation->setStartValue(QPoint(-offsetX + nextPos.x(), -offsetY + nextPos.y()));
	nextPosAnimation->setEndValue(QPoint(nextPos.x(), nextPos.y()));

	animGroup = new QParallelAnimationGroup;
	animGroup->addAnimation(currentPosAnimation);
	animGroup->addAnimation(nextPosAnimation);
	animGroup->addAnimation(currentOpacityAnimation);
	animGroup->addAnimation(nextOpacityAnimation);

	QObject::connect(animGroup, &QParallelAnimationGroup::finished, this, &SlidingStackedWidget::animationDoneSlot);

	nextIndex_ = nextIndex;
	activeIndex_ = fromIndex;
	isAnimating = true;
	animGroup->start(QAbstractAnimation::DeleteWhenStopped);

	nextPageInstant = false;
}

void SlidingStackedWidget::animationDoneSlot()
{
	QStackedWidget::setCurrentIndex(nextIndex_);

	widget(activeIndex_)->hide();
	widget(activeIndex_)->move(activePos_);

	isAnimating = false;

	emit animationFinished();
}
