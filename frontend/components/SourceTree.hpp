#pragma once

#include "SourceTreeItem.hpp"
#include "SourceTreeModel.hpp"

#include <QListView>
#include <QStaticText>
#include <QSvgRenderer>

class SourceTree : public QListView {
	Q_OBJECT

	bool ignoreReorder = false;

	friend class SourceTreeModel;
	friend class SourceTreeItem;

	bool textPrepared = false;
	QStaticText textNoSources;
	QSvgRenderer iconNoSources;

	OBSData undoSceneData;

	SourceTreeModel *treeModel;

	bool iconsVisible = true;

	void UpdateNoSourcesMessage();

	void ResetWidgets();
	void UpdateWidget(const QModelIndex &idx, obs_sceneitem_t *item);
	void UpdateWidgets(bool force = false);

public:
	inline SourceTreeItem *GetItemWidget(int idx)
	{
		QWidget *widget = indexWidget(model()->createIndex(idx, 0));
		return static_cast<SourceTreeItem *>(widget);
	}

	explicit SourceTree(QWidget *parent = nullptr);

	void setModel(SourceTreeModel *model);
	SourceTreeModel *model() const { return treeModel; }

	inline bool IgnoreReorder() const { return ignoreReorder; }
	inline void Clear() { model()->Clear(); }

	inline void Add(obs_sceneitem_t *item) { model()->Add(item); }
	inline OBSSceneItem Get(int idx) { return model()->Get(idx); }
	inline QString GetNewGroupName() { return model()->GetNewGroupName(); }

	void SelectItem(obs_sceneitem_t *sceneitem, bool select);

	bool MultipleBaseSelected() const;
	bool GroupsSelected() const;
	bool GroupedItemsSelected() const;

	void UpdateIcons();
	void SetIconsVisible(bool visible);

public slots:
	inline void ReorderItems() { model()->ReorderItems(); }
	inline void RefreshItems() { model()->SceneChanged(); }
	void Remove(OBSSceneItem item, OBSScene scene);
	void GroupSelectedItems();
	void UngroupSelectedGroups();
	void AddGroup();
	bool Edit(int idx);
	void NewGroupEdit(int idx);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
	virtual void paintEvent(QPaintEvent *event) override;

	virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
};
