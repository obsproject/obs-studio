#pragma once

#include "ui_OBSBasicTransform.h"

#include <obs.hpp>

#include <QDialog>

class OBSBasic;
class QListWidgetItem;

class OBSBasicTransform : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::OBSBasicTransform> ui;

	OBSBasic *main;
	OBSSceneItem item;
	std::vector<OBSSignal> sigs;

	std::string undo_data;

	bool ignoreTransformSignal = false;
	bool ignoreItemChange = false;

	template<typename Widget, typename WidgetParent, typename... SignalArgs, typename... SlotArgs>
	void HookWidget(Widget *widget, void (WidgetParent::*signal)(SignalArgs...),
			void (OBSBasicTransform::*slot)(SlotArgs...))
	{
		QObject::connect(widget, signal, this, slot);
	}

	void SetScene(OBSScene scene);
	void SetItem(OBSSceneItem newItem);

	static void OBSSceneItemTransform(void *param, calldata_t *data);
	static void OBSSceneItemRemoved(void *param, calldata_t *data);
	static void OBSSceneItemSelect(void *param, calldata_t *data);
	static void OBSSceneItemDeselect(void *param, calldata_t *data);
	static void OBSSceneItemLocked(void *param, calldata_t *data);

private slots:
	void RefreshControls();
	void SetItemQt(OBSSceneItem newItem);
	void OnBoundsType(int index);
	void OnControlChanged();
	void OnCropChanged();
	void SetEnabled(bool enable);

public:
	OBSBasicTransform(OBSSceneItem item, OBSBasic *parent);
	~OBSBasicTransform();

public slots:
	void OnSceneChanged(QListWidgetItem *current, QListWidgetItem *prev);
};
