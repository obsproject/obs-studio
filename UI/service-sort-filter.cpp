#include "service-sort-filter.hpp"

#include <QStandardItemModel>

#include "obs-app.hpp"

#define SERVICE_SHOW_ALL QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll")
constexpr const char *CUSTOM_SERVICE_ID = "custom_service";

ServiceSortFilterProxyModel::ServiceSortFilterProxyModel(QComboBox *parent)
	: QSortFilterProxyModel(parent)
{
	setSortCaseSensitivity(Qt::CaseInsensitive);

	// Store text/data combo to be able to find the id while sorting
	for (int i = 0; i < parent->count(); i++)
		items[parent->itemText(i)] = parent->itemData(i);
}

bool ServiceSortFilterProxyModel::lessThan(const QModelIndex &left,
					   const QModelIndex &right) const
{
	QVariant leftData = sourceModel()->data(left);
	QVariant rightData = sourceModel()->data(right);

	// Make Show All last
	if (leftData.toString() == SERVICE_SHOW_ALL)
		return false;
	if (rightData.toString() == SERVICE_SHOW_ALL)
		return true;

	// Make Custom… first
	if (items.value(leftData.toString()) == CUSTOM_SERVICE_ID)
		return true;
	if (items.value(rightData.toString()) == CUSTOM_SERVICE_ID)
		return false;

	return QSortFilterProxyModel::lessThan(left, right);
}
