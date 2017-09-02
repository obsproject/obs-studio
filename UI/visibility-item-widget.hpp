#pragma once

#include <QWidget>
#include <QStyledItemDelegate>
#include <obs.hpp>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class VisibilityCheckBox;
class LockedCheckBox;

class VisibilityItemWidget : public QWidget {
	Q_OBJECT

private:
	OBSSceneItem item;
	OBSSource source;
	QLabel *label = nullptr;
	VisibilityCheckBox *vis = nullptr;
	LockedCheckBox *lock = nullptr;
	QString oldName;

	OBSSignal sceneRemoveSignal;
	OBSSignal enabledSignal;
	OBSSignal renamedSignal;
	bool sceneRemoved = false;

	bool active = false;
	bool selected = false;

	static void OBSSceneRemove(void *param, calldata_t *data);
	static void OBSSceneItemRemove(void *param, calldata_t *data);
	static void OBSSceneItemVisible(void *param, calldata_t *data);
	static void OBSSceneItemLocked(void *param, calldata_t *data);
	static void OBSSourceEnabled(void *param, calldata_t *data);
	static void OBSSourceRenamed(void *param, calldata_t *data);

	void DisconnectItemSignals();

private slots:
	void VisibilityClicked(bool visible);
	void LockClicked(bool locked);
	void SourceEnabled(bool enabled);
	void SourceRenamed(QString name);
	void SourceLocked(bool locked);

public:
	VisibilityItemWidget(obs_source_t *source);
	VisibilityItemWidget(obs_sceneitem_t *item);
	~VisibilityItemWidget();

	void SetColor(const QColor &color, bool active, bool selected);
};

class VisibilityItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	VisibilityItemDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
			const QModelIndex &index) const override;
};

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item,
		obs_source_t *source);
void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item,
		obs_sceneitem_t *sceneItem);
