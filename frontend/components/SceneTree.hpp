#pragma once

#include <QListWidget>
#include <QObject>
#include <QResizeEvent>
#include <QWidget>

class SceneTree : public QListWidget {
	Q_OBJECT
	Q_PROPERTY(int gridItemWidth READ GetGridItemWidth WRITE SetGridItemWidth DESIGNABLE true)
	Q_PROPERTY(int gridItemHeight READ GetGridItemHeight WRITE SetGridItemHeight DESIGNABLE true)

	bool gridMode = false;
	int maxWidth = 150;
	int itemHeight = 24;

public:
	void SetGridMode(bool grid);
	bool GetGridMode();

	void SetGridItemWidth(int width);
	void SetGridItemHeight(int height);
	int GetGridItemWidth();
	int GetGridItemHeight();

	explicit SceneTree(QWidget *parent = nullptr);

private:
	void RepositionGrid(QDragMoveEvent *event = nullptr);

protected:
	virtual bool eventFilter(QObject *obj, QEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual void startDrag(Qt::DropActions supportedActions) override;
	virtual void dropEvent(QDropEvent *event) override;
	virtual void dragMoveEvent(QDragMoveEvent *event) override;
	virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
	virtual void rowsInserted(const QModelIndex &parent, int start, int end) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 3)
	virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
#endif

signals:
	void scenesReordered();
};
