#include "scene-tree.hpp"
#include "obs.h"

#include <QSizePolicy>
#include <QScrollBar>
#include <QDropEvent>
#include <QPushButton>
#include <QTimer>
#include <cmath>
#include <QMimeData>

SceneTree::SceneTree(QWidget *parent_) : QListWidget(parent_)
{
	setAcceptDrops(true);
	installEventFilter(this);
	setDragDropMode(DragDrop);
	setMovement(QListView::Snap);
}

void SceneTree::SetGridMode(bool grid)
{
	parent()->setProperty("gridMode", grid);
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

	QResizeEvent event(size(), size());
	resizeEvent(&event);
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

bool SceneTree::eventFilter(QObject *obj, QEvent *event)
{
	return QObject::eventFilter(obj, event);
}

void SceneTree::resizeEvent(QResizeEvent *event)
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

	QListWidget::resizeEvent(event);
}

void SceneTree::startDrag(Qt::DropActions supportedActions)
{
	QListWidget::startDrag(supportedActions);
}

void SceneTree::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasFormat("application/indexes")) {
		QByteArray encodedData =
			event->mimeData()->data("application/indexes");
		QDataStream stream(&encodedData, QIODevice::ReadOnly);

		QStringList sourceNames;
		stream >> sourceNames;

		QPoint dropPos = event->position().toPoint();
		QListWidgetItem *sceneItem = itemAt(dropPos);

		if (sceneItem) {
			QString sceneName = sceneItem->text();

			for (const QString &sourceName : sourceNames) {
				if (!sceneName.isEmpty() &&
				    !sourceName.isEmpty()) {

					addSourceToScene(sourceName, sceneName);
				}
			}
		}

		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	if (gridMode) {
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

		QPoint point = event->position().toPoint();

		int x = (float)point.x() / wid * std::ceil(wid / maxWidth);
		int y = (point.y() + firstItemY) / itemHeight;

		int r = x + y * std::ceil(wid / maxWidth);

		QListWidgetItem *item = takeItem(selectedIndexes()[0].row());
		insertItem(r, item);
		setCurrentItem(item);
		resize(size());
	}

	QListWidget::dropEvent(event);

	// We must call resizeEvent to correctly place all grid items.
	// We also do this in rowsInserted.
	QResizeEvent resEvent(size(), size());
	SceneTree::resizeEvent(&resEvent);

	QTimer::singleShot(100, [this]() { emit scenesReordered(); });
}

void SceneTree::addSourceToScene(const QString &sourceName,
				 const QString &sceneName)
{
	obs_source_t *source = obs_get_source_by_name(qPrintable(sourceName));
	if (!source) {
		return;
	}
	obs_source_t *sceneSource =
		obs_get_source_by_name(qPrintable(sceneName));
	if (!sceneSource) {
		obs_source_release(source);
		return;
	}

	obs_scene_t *scene = obs_scene_from_source(sceneSource);
	if (!scene) {
		obs_source_release(source);
		obs_source_release(sceneSource);
		return;
	}

	obs_sceneitem_t *sceneItem = obs_scene_add(scene, source);

	obs_source_release(source);
	obs_source_release(sceneSource);
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

		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected())
				continue;

			QModelIndex index = indexFromItem(wItem);

			int off = (i >= r ? 1 : 0) -
				  (i > orig && i > r ? 1 : 0) -
				  (i > orig && i == r ? 2 : 0);

			int xPos = (i + off) % (int)std::ceil(wid / maxWidth);
			int yPos = (i + off) / (int)std::ceil(wid / maxWidth);
			QSize g = gridSize();

			QPoint position(xPos * g.width(), yPos * g.height());
			setPositionForIndex(position, index);
		}
	} else {
		for (int i = 0; i < count(); i++) {
			auto *wItem = item(i);

			if (wItem->isSelected())
				continue;

			QModelIndex index = indexFromItem(wItem);

			int xPos = i % (int)std::ceil(wid / maxWidth);
			int yPos = i / (int)std::ceil(wid / maxWidth);
			QSize g = gridSize();

			QPoint position(xPos * g.width(), yPos * g.height());
			setPositionForIndex(position, index);
		}
	}
}

void SceneTree::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat("application/indexes")) {
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	if (event->source() == this) {
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	QListWidget::dragEnterEvent(event);
}

void SceneTree::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasFormat("application/indexes")) {
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	if (event->source() == this) {
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}

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
	QResizeEvent event(size(), size());
	SceneTree::resizeEvent(&event);

	QListWidget::rowsInserted(parent, start, end);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 3)
// Workaround for QTBUG-105870. Remove once that is solved upstream.
void SceneTree::selectionChanged(const QItemSelection &selected,
				 const QItemSelection &deselected)
{
	if (selected.count() == 0 && deselected.count() > 0 &&
	    !property("clearing").toBool())
		setCurrentRow(deselected.indexes().front().row());
}
#endif
