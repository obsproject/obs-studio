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

#include <QBoxLayout>
#include <QFrame>
#include <QPointer>
#include <QWidget>

namespace idian {

class PopupContents;

struct Anchor {
	enum Side : uint8_t {
		Left = 1 << 0,
		HCenter = 1 << 1,
		Right = 1 << 2,
		Top = 1 << 3,
		VCenter = 1 << 4,
		Bottom = 1 << 5
	};

	using Position = uint8_t;

	static constexpr Position TopLeft = Top | Left;
	static constexpr Position TopCenter = Top | HCenter;
	static constexpr Position TopRight = Top | Right;
	static constexpr Position LeftCenter = Left | VCenter;
	static constexpr Position Center = HCenter | VCenter;
	static constexpr Position RightCenter = Right | VCenter;
	static constexpr Position BottomLeft = Bottom | Left;
	static constexpr Position BottomCenter = Bottom | HCenter;
	static constexpr Position BottomRight = Bottom | Right;

	static void flipHorizontal(Position &position)
	{
		const bool left = position & Anchor::Left;
		const bool right = position & Anchor::Right;

		position &= ~(Anchor::Left | Anchor::Right);

		if (left) {
			position |= Anchor::Right;
		}
		if (right) {
			position |= Anchor::Left;
		}
	}

	static void flipVertical(Position &position)
	{
		const bool top = position & Anchor::Top;
		const bool bottom = position & Anchor::Bottom;

		position &= ~(Anchor::Top | Anchor::Bottom);

		if (top) {
			position |= Anchor::Bottom;
		}
		if (bottom) {
			position |= Anchor::Top;
		}
	}
};

class AbstractPopup : public QWidget {
	Q_OBJECT

	bool multipleSteps = false;

	QPointer<QBoxLayout> mainLayout{};

	QPoint anchorPoint{0, 0};

	QList<QPointer<QWidget>> ancestorWidgets;
	void clearAncestorFilters();

	std::optional<QPoint> fixedAnchor{std::nullopt};

	Anchor::Position anchorLocation = Anchor::TopLeft;
	Anchor::Position offsetLocation = Anchor::TopLeft;

	QPoint calculateOffset(Anchor::Position arrowLocation);

	bool isUpdating = false;

	QPointer<QTimer> positionTimer;
	void updatePosition();
	void updateVisibility();

protected:
	QPointer<QWidget> anchorWidget{};
	QPointer<PopupContents> innerFrame{};

	bool eventFilter(QObject *obj, QEvent *ev) override;

public:
	AbstractPopup(QWidget *parent);
	~AbstractPopup();

	void queuePositionUpdate();

	void setAnchorPoint(QPoint point);
	void setAnchorFrom(Anchor::Position corner);

	void setAnchorWidget(QWidget *anchor);
	void setAnchorWidget(QWidget *anchor, Anchor::Position location);
	void setAnchorFixed(QPoint point);

	void updateWidgetAnchorPosition();
	QPoint getAnchorPoint();

	void setWidget(QWidget *widget);

	void setOrientation(Qt::Orientation orientation_);

public slots:
	void show();
};
} // namespace idian
