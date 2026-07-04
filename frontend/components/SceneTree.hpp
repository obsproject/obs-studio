#pragma once

#include <QListWidget>
#include <QObject>
#include <QResizeEvent>
#include <QWidget>

class SceneTree : public QListWidget {
	Q_OBJECT
	Q_PROPERTY(int gridItemWidth READ getGridItemWidth WRITE setGridItemWidth DESIGNABLE true)
	Q_PROPERTY(int gridItemHeight READ getGridItemHeight WRITE setGridItemHeight DESIGNABLE true)

	bool gridMode = false;
	int maxWidth = 150;
	int itemHeight = 24;
	int lastTargetRow = -1;

public:
	void setGridMode(bool grid);
	bool getGridMode();

	void setGridItemWidth(int width);
	void setGridItemHeight(int height);
	int getGridItemWidth();
	int getGridItemHeight();

	explicit SceneTree(QWidget *parent = nullptr);

private:
	void recalculateGridSize();
	void repositionGrid(QDragMoveEvent *event = nullptr);

protected:
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void startDrag(Qt::DropActions supportedActions) override;
	virtual void dropEvent(QDropEvent *event) override;
	virtual void dragMoveEvent(QDragMoveEvent *event) override;
	virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
	virtual void rowsInserted(const QModelIndex &parent, int start, int end) override;
	virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

signals:
	void scenesReordered();
};
