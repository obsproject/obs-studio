#pragma once

#include <obs-frontend-api.h>
#include <obs.hpp>

#include <QAbstractListModel>

class SourceTree;

class SourceTreeModel : public QAbstractListModel {
	Q_OBJECT

	friend class SourceTree;
	friend class SourceTreeItem;

	SourceTree *st;
	QVector<OBSSceneItem> items;
	bool hasGroups = false;

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);
	void Clear();
	void SceneChanged();
	void ReorderItems();

	void Add(obs_sceneitem_t *item);
	void Remove(obs_sceneitem_t *item);
	OBSSceneItem Get(int idx);
	QString GetNewGroupName();
	void AddGroup();

	void GroupSelectedItems(QModelIndexList &indices);
	void UngroupSelectedGroups(QModelIndexList &indices);

	void ExpandGroup(obs_sceneitem_t *item);
	void CollapseGroup(obs_sceneitem_t *item);

	void UpdateGroupState(bool update);

public:
	explicit SourceTreeModel(SourceTree *st);

	virtual int rowCount(const QModelIndex &parent) const override;
	virtual QVariant data(const QModelIndex &index, int role) const override;

	virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
	virtual Qt::DropActions supportedDropActions() const override;
};
