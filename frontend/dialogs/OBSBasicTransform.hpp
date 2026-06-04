#pragma once

#include "ui_OBSBasicTransform.h"

#include <obs.hpp>
#include <components/AlignmentSelector.hpp>

#include <QDialog>
#include <QPointer>

class OBSBasic;
class QListWidgetItem;

class OBSBasicTransform : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::OBSBasicTransform> ui;

	QPointer<AlignmentSelector> positionAlignment;
	QPointer<AlignmentSelector> boundsAlignment;

	OBSBasic *main;
	OBSSceneItem item;
	std::vector<OBSSignal> sigs;

	std::string undo_data;

	bool ignoreTransformSignal = false;
	bool ignoreItemChange = false;

	template<typename Widget, typename WidgetParent, typename... SignalArgs, typename... SlotArgs>
	void hookWidget(Widget *widget, void (WidgetParent::*signal)(SignalArgs...),
			void (OBSBasicTransform::*slot)(SlotArgs...))
	{
		QObject::connect(widget, signal, this, slot);
	}

	void setScene(OBSScene scene);
	void setItem(OBSSceneItem newItem);

	static void OBSSceneItemTransform(void *param, calldata_t *data);
	static void OBSSceneItemRemoved(void *param, calldata_t *data);
	static void OBSSceneItemSelect(void *param, calldata_t *data);
	static void OBSSceneItemDeselect(void *param, calldata_t *data);
	static void OBSSceneItemLocked(void *param, calldata_t *data);

private slots:
	void refreshControls();
	void setItemQt(OBSSceneItem newItem);
	void onAlignChanged(int index);
	void onBoundsType(int index);
	void onControlChanged();
	void onCropChanged();
	void setEnabled(bool enable);

public:
	OBSBasicTransform(OBSSceneItem item, OBSBasic *parent);
	~OBSBasicTransform();

public slots:
	void onSceneChanged(QListWidgetItem *current, QListWidgetItem *prev);
};
