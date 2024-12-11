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

	bool iconsVisible = true;

	void UpdateNoSourcesMessage();

	void ResetWidgets();
	void UpdateWidget(const QModelIndex &idx, obs_sceneitem_t *item);
	void UpdateWidgets(bool force = false);

	inline SourceTreeModel *GetStm() const { return reinterpret_cast<SourceTreeModel *>(model()); }

public:
	inline SourceTreeItem *GetItemWidget(int idx)
	{
		QWidget *widget = indexWidget(GetStm()->createIndex(idx, 0));
		return reinterpret_cast<SourceTreeItem *>(widget);
	}

	explicit SourceTree(QWidget *parent = nullptr);

	inline bool IgnoreReorder() const { return ignoreReorder; }
	inline void Clear() { GetStm()->Clear(); }

	inline void Add(obs_sceneitem_t *item) { GetStm()->Add(item); }
	inline OBSSceneItem Get(int idx) { return GetStm()->Get(idx); }
	inline QString GetNewGroupName() { return GetStm()->GetNewGroupName(); }

	void SelectItem(obs_sceneitem_t *sceneitem, bool select);

	bool MultipleBaseSelected() const;
	bool GroupsSelected() const;
	bool GroupedItemsSelected() const;

	void UpdateIcons();
	void SetIconsVisible(bool visible);

public slots:
	inline void ReorderItems() { GetStm()->ReorderItems(); }
	inline void RefreshItems() { GetStm()->SceneChanged(); }
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
