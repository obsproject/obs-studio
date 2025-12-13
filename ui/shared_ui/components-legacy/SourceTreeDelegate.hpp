#pragma once

#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class SourceTreeDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	SourceTreeDelegate(QObject *parent);
	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
