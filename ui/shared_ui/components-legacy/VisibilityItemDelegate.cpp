#include "VisibilityItemWidget.hpp"
#include "VisibilityItemDelegate.hpp"

#include <QKeyEvent>

#include "moc_VisibilityItemDelegate.cpp"

VisibilityItemDelegate::VisibilityItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void VisibilityItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
				   const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);

	QObject *parentObj = parent();
	QListWidget *list = qobject_cast<QListWidget *>(parentObj);
	if (!list)
		return;

	QListWidgetItem *item = list->item(index.row());
	VisibilityItemWidget *widget = qobject_cast<VisibilityItemWidget *>(list->itemWidget(item));
	if (!widget)
		return;

	bool selected = option.state.testFlag(QStyle::State_Selected);
	bool active = option.state.testFlag(QStyle::State_Active);

	QPalette palette = list->palette();
#if defined(_WIN32) || defined(__APPLE__)
	QPalette::ColorGroup group = active ? QPalette::Active : QPalette::Inactive;
#else
	QPalette::ColorGroup group = QPalette::Active;
#endif

#ifdef _WIN32
	QPalette::ColorRole highlightRole = QPalette::WindowText;
#else
	QPalette::ColorRole highlightRole = QPalette::HighlightedText;
#endif

	QPalette::ColorRole role;

	if (selected && active)
		role = highlightRole;
	else
		role = QPalette::WindowText;

	widget->SetColor(palette.color(group, role), active, selected);
}

bool VisibilityItemDelegate::eventFilter(QObject *object, QEvent *event)
{
	QWidget *editor = qobject_cast<QWidget *>(object);
	if (!editor)
		return false;

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
			return false;
		}
	}

	return QStyledItemDelegate::eventFilter(object, event);
}
