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

#include <components/AlignmentSelector.hpp>

#include <QAccessible>
#include <QAccessibleWidget>
#include <QAccessibleInterface>

class AccessibleAlignmentCell;

class AccessibleAlignmentSelector : public QAccessibleWidget {

	mutable QHash<int, QAccessible::Id> cellInterfaces{};

public:
	explicit AccessibleAlignmentSelector(AlignmentSelector *widget);
	~AccessibleAlignmentSelector();

	QObject *object() const override { return widget; }
	QRect rect() const override;
	QAccessible::Role role() const override;
	QAccessible::State state() const override;
	QString text(QAccessible::Text t) const override;
	QAccessibleInterface *child(int index) const override;
	int childCount() const override;
	int indexOfChild(const QAccessibleInterface *child) const override;
	bool isValid() const override;
	QAccessibleInterface *focusChild() const override;

	QVariant value() const;

	AlignmentSelector *widget;
};

class AccessibleAlignmentCell : public QAccessibleInterface {
	QAccessibleInterface *parent_;
	AlignmentSelector *widget;
	int index_;

public:
	AccessibleAlignmentCell(QAccessibleInterface *parent, AlignmentSelector *widget, int index);

	int index() const { return index_; }

	QRect rect() const override;
	QString text(QAccessible::Text t) const override;
	QAccessible::State state() const override;
	QAccessible::Role role() const override;

	QObject *object() const override { return nullptr; }
	QAccessibleInterface *child(int) const override { return nullptr; }
	QAccessibleInterface *childAt(int, int) const override { return nullptr; }
	int childCount() const override { return 0; }
	int indexOfChild(const QAccessibleInterface *) const override { return -1; }
	QAccessibleInterface *parent() const override { return parent_; }
	QAccessibleInterface *focusChild() const override { return nullptr; }
	bool isValid() const override { return widget != nullptr; }
	void setText(QAccessible::Text, const QString &) override {}
};
