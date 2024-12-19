#include "SourceTree.hpp"
#include "SourceTreeDelegate.hpp"
#include "moc_SourceTreeDelegate.cpp"

SourceTreeDelegate::SourceTreeDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize SourceTreeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	SourceTree *tree = qobject_cast<SourceTree *>(parent());
	QWidget *item = tree->indexWidget(index);

	if (!item)
		return (QSize(0, 0));

	return (QSize(option.widget->minimumWidth(), item->height()));
}
