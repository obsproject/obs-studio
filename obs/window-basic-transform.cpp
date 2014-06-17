#include "window-basic-transform.hpp"
#include "window-basic-main.hpp"

Q_DECLARE_METATYPE(OBSSceneItem);

static OBSSceneItem FindASelectedItem(OBSScene scene)
{
	auto func = [] (obs_scene_t scene, obs_sceneitem_t item, void *param)
	{
		OBSSceneItem &dst = *reinterpret_cast<OBSSceneItem*>(param);

		if (obs_sceneitem_selected(item)) {
			dst = item;
			return false;
		}

		UNUSED_PARAMETER(scene);
		return true;
	};

	OBSSceneItem item;
	obs_scene_enum_items(scene, func, &item);
	return item;
}

void OBSBasicTransform::HookWidget(QWidget *widget, const char *signal,
		const char *slot)
{
	QObject::connect(widget, signal, this, slot);
}

#define COMBO_CHANGED   SIGNAL(currentIndexChanged(int))
#define DSCROLL_CHANGED SIGNAL(valueChanged(double))

OBSBasicTransform::OBSBasicTransform(OBSBasic *parent)
	: QDialog (parent),
	  ui      (new Ui::OBSBasicTransform),
	  main    (parent)
{
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	HookWidget(ui->positionX,    DSCROLL_CHANGED, SLOT(OnControlChanged()));
	HookWidget(ui->positionY,    DSCROLL_CHANGED, SLOT(OnControlChanged()));
	HookWidget(ui->rotation,     DSCROLL_CHANGED, SLOT(OnControlChanged()));
	HookWidget(ui->scaleX,       DSCROLL_CHANGED, SLOT(OnControlChanged()));
	HookWidget(ui->scaleY,       DSCROLL_CHANGED, SLOT(OnControlChanged()));
	HookWidget(ui->align,        COMBO_CHANGED,   SLOT(OnControlChanged()));
	HookWidget(ui->boundsType,   COMBO_CHANGED,   SLOT(OnBoundsType(int)));
	HookWidget(ui->boundsAlign,  COMBO_CHANGED,   SLOT(OnControlChanged()));
	HookWidget(ui->boundsWidth,  DSCROLL_CHANGED, SLOT(OnControlChanged()));
	HookWidget(ui->boundsHeight, DSCROLL_CHANGED, SLOT(OnControlChanged()));

	OBSScene curScene = main->GetCurrentScene();
	SetScene(curScene);
	SetItem(FindASelectedItem(curScene));

	channelChangedSignal.Connect(obs_signalhandler(), "channel_change",
			OBSChannelChanged, this);
}

void OBSBasicTransform::SetScene(OBSScene scene)
{
	transformSignal.Disconnect();
	selectSignal.Disconnect();
	deselectSignal.Disconnect();
	removeSignal.Disconnect();

	if (scene) {
		OBSSource source = obs_scene_getsource(scene);
		signal_handler_t signal = obs_source_signalhandler(source);

		transformSignal.Connect(signal, "item_transform",
				OBSSceneItemTransform, this);
		removeSignal.Connect(signal, "item_remove",
				OBSSceneItemRemoved, this);
		selectSignal.Connect(signal, "item_select",
				OBSSceneItemSelect, this);
		deselectSignal.Connect(signal, "item_deselect",
				OBSSceneItemDeselect, this);
	}
}

void OBSBasicTransform::SetItem(OBSSceneItem newItem)
{
	QMetaObject::invokeMethod(this, "SetItemQt",
			Q_ARG(OBSSceneItem, OBSSceneItem(newItem)));
}

void OBSBasicTransform::SetItemQt(OBSSceneItem newItem)
{
	item = newItem;
	if (item)
		RefreshControls();

	setEnabled(!!item);
}

void OBSBasicTransform::OBSChannelChanged(void *param, calldata_t data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform*>(param);
	uint32_t channel = (uint32_t)calldata_int(data, "channel");
	OBSSource source = (obs_source_t)calldata_ptr(data, "source");

	if (channel == 0) {
		OBSScene scene = obs_scene_fromsource(source);
		window->SetScene(scene);

		if (!scene)
			window->SetItem(nullptr);
		else
			window->SetItem(FindASelectedItem(scene));
	}
}

void OBSBasicTransform::OBSSceneItemTransform(void *param, calldata_t data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform*>(param);
	OBSSceneItem item = (obs_sceneitem_t)calldata_ptr(data, "item");

	if (item == window->item && !window->ignoreTransformSignal)
		QMetaObject::invokeMethod(window, "RefreshControls");
}

void OBSBasicTransform::OBSSceneItemRemoved(void *param, calldata_t data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform*>(param);
	OBSScene     scene = (obs_scene_t)calldata_ptr(data, "scene");
	OBSSceneItem item = (obs_sceneitem_t)calldata_ptr(data, "item");

	if (item == window->item)
		window->SetItem(FindASelectedItem(scene));
}

void OBSBasicTransform::OBSSceneItemSelect(void *param, calldata_t data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform*>(param);
	OBSSceneItem item  = (obs_sceneitem_t)calldata_ptr(data, "item");

	if (item != window->item)
		window->SetItem(item);
}

void OBSBasicTransform::OBSSceneItemDeselect(void *param, calldata_t data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform*>(param);
	OBSScene     scene = (obs_scene_t)calldata_ptr(data, "scene");
	OBSSceneItem item  = (obs_sceneitem_t)calldata_ptr(data, "item");

	if (item == window->item)
		window->SetItem(FindASelectedItem(scene));
}

static const uint32_t listToAlign[] = {
	OBS_ALIGN_TOP | OBS_ALIGN_LEFT,
	OBS_ALIGN_TOP,
	OBS_ALIGN_TOP | OBS_ALIGN_RIGHT,
	OBS_ALIGN_LEFT,
	OBS_ALIGN_CENTER,
	OBS_ALIGN_RIGHT,
	OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT,
	OBS_ALIGN_BOTTOM,
	OBS_ALIGN_BOTTOM | OBS_ALIGN_RIGHT
};

static int AlignToList(uint32_t align)
{
	int index = 0;
	for (uint32_t curAlign : listToAlign) {
		if (curAlign == align)
			return index;

		index++;
	}

	return 0;
}

void OBSBasicTransform::RefreshControls()
{
	if (!item)
		return;

	obs_sceneitem_info osi;
	obs_sceneitem_get_info(item, &osi);

	int alignIndex       = AlignToList(osi.alignment);
	int boundsAlignIndex = AlignToList(osi.bounds_alignment);

	ignoreItemChange = true;
	ui->positionX->setValue(osi.pos.x);
	ui->positionY->setValue(osi.pos.y);
	ui->rotation->setValue(osi.rot);
	ui->scaleX->setValue(osi.scale.x);
	ui->scaleY->setValue(osi.scale.y);
	ui->align->setCurrentIndex(alignIndex);

	ui->boundsType->setCurrentIndex(int(osi.bounds_type));
	ui->boundsAlign->setCurrentIndex(boundsAlignIndex);
	ui->boundsWidth->setValue(osi.bounds.x);
	ui->boundsHeight->setValue(osi.bounds.y);
	ignoreItemChange = false;
}

void OBSBasicTransform::OnBoundsType(int index)
{
	if (index == -1)
		return;

	obs_bounds_type type   = (obs_bounds_type)index;
	bool            enable = (type != OBS_BOUNDS_NONE);

	ui->boundsAlign->setEnabled(enable);
	ui->boundsWidth->setEnabled(enable);
	ui->boundsHeight->setEnabled(enable);

	if (!ignoreItemChange) {
		obs_bounds_type lastType = obs_sceneitem_get_bounds_type(item);
		if (lastType == OBS_BOUNDS_NONE) {
			OBSSource source = obs_sceneitem_getsource(item);
			int width  = (int)obs_source_getwidth(source);
			int height = (int)obs_source_getheight(source);

			ui->boundsWidth->setValue(width);
			ui->boundsHeight->setValue(height);
		}
	}

	OnControlChanged();
}

void OBSBasicTransform::OnControlChanged()
{
	if (ignoreItemChange)
		return;

	obs_sceneitem_info osi;
	osi.pos.x            = float(ui->positionX->value());
	osi.pos.y            = float(ui->positionY->value());
	osi.rot              = float(ui->rotation->value());
	osi.scale.x          = float(ui->scaleX->value());
	osi.scale.y          = float(ui->scaleY->value());
	osi.alignment        = listToAlign[ui->align->currentIndex()];

	osi.bounds_type      = (obs_bounds_type)ui->boundsType->currentIndex();
	osi.bounds_alignment = listToAlign[ui->boundsAlign->currentIndex()];
	osi.bounds.x         = float(ui->boundsWidth->value());
	osi.bounds.y         = float(ui->boundsHeight->value());

	ignoreTransformSignal = true;
	obs_sceneitem_set_info(item, &osi);
	ignoreTransformSignal = false;
}
