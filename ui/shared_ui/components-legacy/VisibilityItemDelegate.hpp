#pragma once

#include <QStyledItemDelegate>

class QObject;

class VisibilityItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	VisibilityItemDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	bool eventFilter(QObject *object, QEvent *event) override;
};
