#include "SceneTree.hpp"

#include <QScrollBar>

#include "moc_SceneTree.cpp"

SceneTree::SceneTree(QWidget *parent_) : QListWidget(parent_)
{
	setDragDropMode(InternalMove);
	setMovement(QListView::Snap);
}

void SceneTree::SetGridMode(bool grid)
{
	parent()->setProperty("class", grid ? "list-grid" : "");
	gridMode = grid;

	if (gridMode) {
		setResizeMode(QListView::Adjust);
		setViewMode(QListView::IconMode);
		setUniformItemSizes(true);
		setStyleSheet("*{padding: 0; margin: 0;}");
	} else {
		setViewMode(QListView::ListMode);
		setResizeMode(QListView::Fixed);
		setStyleSheet("");
	}

	recalculateGridSize();
}

bool SceneTree::GetGridMode()
{
	return gridMode;
}

void SceneTree::SetGridItemWidth(int width)
{
	maxWidth = width;
}

void SceneTree::SetGridItemHeight(int height)
{
	itemHeight = height;
}

int SceneTree::GetGridItemWidth()
{
	return maxWidth;
}

int SceneTree::GetGridItemHeight()
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
		QSignalBlocker block(this);

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
		int scrollWid = verticalScrollBar()->sizeHint().width();
		const QRect lastItem = visualItemRect(item(count() - 1));
		const int h = lastItem.y() + lastItem.height();

		if (h < height()) {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			scrollWid = 0;
		} else {
			setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		}

		int wid = contentsRect().width() - scrollWid - 1;
		int items = (int)std::ceil((float)wid / maxWidth);
		int itemWidth = wid / items;

		setGridSize(QSize(itemWidth, itemHeight));

		for (int i = 0; i < count(); i++) {
			item(i)->setSizeHint(QSize(itemWidth, itemHeight));
		}
	} else {
		setGridSize(QSize());
		for (int i = 0; i < count(); i++) {
			item(i)->setData(Qt::SizeHintRole, QVariant());
		}
	}
}

void SceneTree::RepositionGrid(QDragMoveEvent *event)
{
	int scrollWid = verticalScrollBar()->sizeHint().width();
	const QRect firstItem = visualItemRect(item(0));
	const QRect lastItem = visualItemRect(item(count() - 1));
	const int h = lastItem.y() + lastItem.height();
	const int firstItemY = abs(firstItem.y());

	if (h < height()) {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scrollWid = 0;
	} else {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	}

	float wid = contentsRect().width() - scrollWid - 1;

	if (event) {
		QPoint point = event->position().toPoint();

		int x = (float)point.x() / wid * std::ceil(wid / maxWidth);
		int y = (point.y() + firstItemY) / itemHeight;

		int r = x + y * std::ceil(wid / maxWidth);
		int orig = selectedIndexes()[0].row();

		lastTargetRow = r;

		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected()) {
				continue;
			}

			QModelIndex index = indexFromItem(wItem);

			int off = (i >= r ? 1 : 0) - (i > orig && i > r ? 1 : 0) - (i > orig && i == r ? 2 : 0);

			int xPos = (i + off) % (int)std::ceil(wid / maxWidth);
			int yPos = (i + off) / (int)std::ceil(wid / maxWidth);
			QSize g = gridSize();

			QPoint position(xPos * g.width(), yPos * g.height());
			setPositionForIndex(position, index);
		}
	} else {
		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected()) {
				continue;
			}

			QModelIndex index = indexFromItem(wItem);

			int xPos = i % (int)std::ceil(wid / maxWidth);
			int yPos = i / (int)std::ceil(wid / maxWidth);
			QSize g = gridSize();

			QPoint position(xPos * g.width(), yPos * g.height());
			setPositionForIndex(position, index);
		}
	}
}

void SceneTree::dragMoveEvent(QDragMoveEvent *event)
{
	if (gridMode) {
		RepositionGrid(event);
	}

	QListWidget::dragMoveEvent(event);
}

void SceneTree::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (gridMode) {
		RepositionGrid();
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
