#pragma once

#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QHash>

class ServiceSortFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
	QHash<QString, QVariant> items;

public:
	ServiceSortFilterProxyModel(QComboBox *parent);

protected:
	bool lessThan(const QModelIndex &left,
		      const QModelIndex &right) const override;
};
