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

#pragma once

#include <QStackedWidget>
#include <QEasingCurve>
#include <QParallelAnimationGroup>

class SlidingStackedWidget : public QStackedWidget {
	Q_OBJECT

public:
	enum SlideDirection { LEFT_TO_RIGHT, RIGHT_TO_LEFT, TOP_TO_BOTTOM, BOTTOM_TO_TOP, AUTOMATIC };

	explicit SlidingStackedWidget(QWidget *parent);

	void setDuration(int duration);
	int duration();
	void setEasingCurve(enum QEasingCurve::Type easingCurve);
	void setVertical(bool vertical = true);

	void setNextPageInstantly();
	void advanceToIndex(int idx, enum SlideDirection direction = AUTOMATIC);
	void advanceToWidget(QWidget *widget, enum SlideDirection direction = AUTOMATIC);

	void setWrap(bool wrap);

public slots:
	void setCurrentIndex(int idx);
	void setCurrentWidget(QWidget *widget);

	bool advanceNext();
	bool advancePrevious();

signals:
	void animationFinished(void);

protected:
	bool nextPageInstant = false;
	bool isAnimating = false;

	int duration_ = 340;

	bool vertical_ = false;
	bool wrap_ = false;
	enum QEasingCurve::Type easingCurve_ = QEasingCurve::OutQuad;

	int activeIndex_ = 0;
	QPoint activePos_ = QPoint(0, 0);

	int nextIndex_ = 0;

	QList<QWidget *> blockedPageList;
	QParallelAnimationGroup *animGroup = nullptr;

protected slots:
	void animationDoneSlot(void);
};
