#include "SceneTree.hpp"

#include <QScrollBar>

#include "moc_SceneTree.cpp"

SceneTree::SceneTree(QWidget *parent_) : QListWidget(parent_)
{
	setDragDropMode(InternalMove);
	setMovement(QListView::Snap);
}

void SceneTree::setGridMode(bool grid)
{
	parent()->setProperty("class", grid ? "list-grid" : "");
	gridMode = grid;

	if (gridMode) {
		setResizeMode(QListView::Adjust);
		setViewMode(QListView::IconMode);
		setUniformItemSizes(true);
	} else {
		setViewMode(QListView::ListMode);
		setResizeMode(QListView::Fixed);
	}

	style()->polish(this);

	recalculateGridSize();
}

bool SceneTree::getGridMode()
{
	return gridMode;
}

void SceneTree::setGridItemWidth(int width)
{
	maxWidth = std::max(1, width);
}

void SceneTree::setGridItemHeight(int height)
{
	itemHeight = std::max(1, height);
}

int SceneTree::getGridItemWidth()
{
	return maxWidth;
}

int SceneTree::getGridItemHeight()
{
	return itemHeight;
}

void SceneTree::resizeEvent(QResizeEvent *event)
{
	recalculateGridSize();

	QListWidget::resizeEvent(event);
}

void SceneTree::startDrag(Qt::DropActions supportedActions)
{
	QListWidget::startDrag(supportedActions);
}

void SceneTree::dropEvent(QDropEvent *event)
{
	if (event->source() != this) {
		QListWidget::dropEvent(event);
		return;
	}

	if (gridMode) {
		QSignalBlocker block{this};

		QListWidgetItem *draggedItem = takeItem(selectedIndexes().first().row());
		if (!draggedItem) {
			return;
		}

		insertItem(lastTargetRow, draggedItem);
		setCurrentItem(draggedItem);

		lastTargetRow = -1;
	}

	QListWidget::dropEvent(event);

	recalculateGridSize();

	emit scenesReordered();
}

void SceneTree::recalculateGridSize()
{
	if (gridMode) {
		int scrollWidth = verticalScrollBar()->sizeHint().width();
		const QRect lastItem = visualItemRect(item(count() - 1));
		const int totalHeight = lastItem.y() + lastItem.height();

		if (totalHeight < height()) {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			scrollWidth = 0;
		} else {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		}

		int width = contentsRect().width() - scrollWidth - 1;
		int items = static_cast<int>(std::ceil(static_cast<float>(width) / maxWidth));
		int itemWidth = width / items;

		setGridSize(QSize{itemWidth, itemHeight});

		for (int i = 0; i < count(); i++) {
			item(i)->setSizeHint(QSize{itemWidth, itemHeight});
		}
	} else {
		setGridSize(QSize{});
		for (int i = 0; i < count(); i++) {
			item(i)->setData(Qt::SizeHintRole, QVariant());
		}
	}
}

void SceneTree::repositionGrid(QDragMoveEvent *event)
{
	int scrollWidth = verticalScrollBar()->sizeHint().width();
	const QRect firstItemRect = visualItemRect(item(0));
	const QRect lastItemRect = visualItemRect(item(count() - 1));
	const int totalHeight = lastItemRect.y() + lastItemRect.height();
	const int firstItemY = abs(firstItemRect.y());

	if (totalHeight < height()) {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scrollWidth = 0;
	} else {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	}

	float width = contentsRect().width() - scrollWidth - 1;

	if (event) {
		QPoint point = event->position().toPoint();

		int x = (float)point.x() / width * std::ceil(width / maxWidth);
		int y = (point.y() + firstItemY) / itemHeight;

		int r = x + y * std::ceil(width / maxWidth);
		int orig = selectedIndexes()[0].row();

		lastTargetRow = r;

		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected()) {
				continue;
			}

			QModelIndex index = indexFromItem(wItem);

			int off = (i >= r ? 1 : 0) - (i > orig && i > r ? 1 : 0) - (i > orig && i == r ? 2 : 0);

			int xPos = (i + off) % static_cast<int>(std::ceil(width / maxWidth));
			int yPos = (i + off) / static_cast<int>(std::ceil(width / maxWidth));
			QSize gridSize_ = gridSize();

			QPoint newPosition{xPos * gridSize_.width(), yPos * gridSize_.height()};
			setPositionForIndex(newPosition, index);
		}
	} else {
		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected()) {
				continue;
			}

			QModelIndex index = indexFromItem(wItem);

			int xPos = i % static_cast<int>(std::ceil(width / maxWidth));
			int yPos = i / static_cast<int>(std::ceil(width / maxWidth));
			QSize gridSize_ = gridSize();

			QPoint newPosition{xPos * gridSize_.width(), yPos * gridSize_.height()};
			setPositionForIndex(newPosition, index);
		}
	}
}

void SceneTree::dragMoveEvent(QDragMoveEvent *event)
{
	if (gridMode) {
		repositionGrid(event);
	}

	QListWidget::dragMoveEvent(event);
}

void SceneTree::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (gridMode) {
		repositionGrid();
	}

	QListWidget::dragLeaveEvent(event);
}

void SceneTree::rowsInserted(const QModelIndex &parent, int start, int end)
{
	recalculateGridSize();

	QListWidget::rowsInserted(parent, start, end);
}

void SceneTree::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	if (selected.count() == 0 && deselected.count() > 0 && !property("clearing").toBool()) {
		setCurrentRow(deselected.indexes().front().row());
	}
}
