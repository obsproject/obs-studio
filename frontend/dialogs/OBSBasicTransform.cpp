#include "OBSBasicTransform.hpp"

#include <widgets/OBSBasic.hpp>

#include "moc_OBSBasicTransform.cpp"

static bool find_sel(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	OBSSceneItem &dst = *reinterpret_cast<OBSSceneItem *>(param);

	if (obs_sceneitem_selected(item)) {
		dst = item;
		return false;
	}
	if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, find_sel, param);
		if (!!dst) {
			return false;
		}
	}

	return true;
};

static OBSSceneItem FindASelectedItem(obs_scene_t *scene)
{
	OBSSceneItem item;
	obs_scene_enum_items(scene, find_sel, &item);
	return item;
}

#define COMBO_CHANGED &QComboBox::currentIndexChanged
#define ISCROLL_CHANGED &QSpinBox::valueChanged
#define DSCROLL_CHANGED &QDoubleSpinBox::valueChanged

OBSBasicTransform::OBSBasicTransform(OBSSceneItem item, OBSBasic *parent)
	: QDialog(parent),
	  ui(new Ui::OBSBasicTransform),
	  main(parent)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	HookWidget(ui->positionX, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->positionY, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->rotation, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->sizeX, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->sizeY, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->align, COMBO_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->boundsType, COMBO_CHANGED, &OBSBasicTransform::OnBoundsType);
	HookWidget(ui->boundsAlign, COMBO_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->boundsWidth, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->boundsHeight, DSCROLL_CHANGED, &OBSBasicTransform::OnControlChanged);
	HookWidget(ui->cropLeft, ISCROLL_CHANGED, &OBSBasicTransform::OnCropChanged);
	HookWidget(ui->cropRight, ISCROLL_CHANGED, &OBSBasicTransform::OnCropChanged);
	HookWidget(ui->cropTop, ISCROLL_CHANGED, &OBSBasicTransform::OnCropChanged);
	HookWidget(ui->cropBottom, ISCROLL_CHANGED, &OBSBasicTransform::OnCropChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	HookWidget(ui->cropToBounds, &QCheckBox::checkStateChanged, &OBSBasicTransform::OnControlChanged);
#else
	HookWidget(ui->cropToBounds, &QCheckBox::stateChanged, &OBSBasicTransform::OnControlChanged);
#endif
	ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

	connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, main,
		&OBSBasic::on_actionResetTransform_triggered);

	installEventFilter(CreateShortcutFilter());

	OBSScene scene = obs_sceneitem_get_scene(item);
	SetScene(scene);
	SetItem(item);

	std::string name = obs_source_get_name(obs_sceneitem_get_source(item));
	setWindowTitle(QTStr("Basic.TransformWindow.Title").arg(name.c_str()));

	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(main->GetCurrentScene(), false);
	undo_data = std::string(obs_data_get_json(wrapper));

	channelChangedSignal.Connect(obs_get_signal_handler(), "channel_change", OBSChannelChanged, this);
}

OBSBasicTransform::~OBSBasicTransform()
{
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(main->GetCurrentScene(), false);

	auto undo_redo = [](const std::string &data) {
		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
		reinterpret_cast<OBSBasic *>(App()->GetMainWindow())->SetCurrentScene(source.Get(), true);
		obs_scene_load_transform_states(data.c_str());
	};

	std::string redo_data(obs_data_get_json(wrapper));
	if (undo_data.compare(redo_data) != 0)
		main->undo_s.add_action(
			QTStr("Undo.Transform").arg(obs_source_get_name(obs_scene_get_source(main->GetCurrentScene()))),
			undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasicTransform::SetScene(OBSScene scene)
{
	sigs.clear();

	if (scene) {
		OBSSource source = obs_scene_get_source(scene);
		signal_handler_t *signal = obs_source_get_signal_handler(source);

		sigs.emplace_back(signal, "item_transform", OBSSceneItemTransform, this);
		sigs.emplace_back(signal, "item_remove", OBSSceneItemRemoved, this);
		sigs.emplace_back(signal, "item_select", OBSSceneItemSelect, this);
		sigs.emplace_back(signal, "item_deselect", OBSSceneItemDeselect, this);
		sigs.emplace_back(signal, "item_locked", OBSSceneItemLocked, this);
	}
}

void OBSBasicTransform::SetItem(OBSSceneItem newItem)
{
	QMetaObject::invokeMethod(this, "SetItemQt", Q_ARG(OBSSceneItem, OBSSceneItem(newItem)));
}

void OBSBasicTransform::SetEnabled(bool enable)
{
	ui->container->setEnabled(enable);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(enable);
}

void OBSBasicTransform::SetItemQt(OBSSceneItem newItem)
{
	item = newItem;
	if (item)
		RefreshControls();

	bool enable = !!item && !obs_sceneitem_locked(item);
	SetEnabled(enable);
}

void OBSBasicTransform::OBSChannelChanged(void *param, calldata_t *data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform *>(param);
	uint32_t channel = (uint32_t)calldata_int(data, "channel");
	OBSSource source = (obs_source_t *)calldata_ptr(data, "source");

	if (channel == 0) {
		OBSScene scene = obs_scene_from_source(source);
		window->SetScene(scene);

		if (!scene)
			window->SetItem(nullptr);
		else
			window->SetItem(FindASelectedItem(scene));
	}
}

void OBSBasicTransform::OBSSceneItemTransform(void *param, calldata_t *data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform *>(param);
	OBSSceneItem item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item == window->item && !window->ignoreTransformSignal)
		QMetaObject::invokeMethod(window, "RefreshControls");
}

void OBSBasicTransform::OBSSceneItemRemoved(void *param, calldata_t *data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform *>(param);
	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(data, "scene");
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item == window->item)
		window->SetItem(FindASelectedItem(scene));
}

void OBSBasicTransform::OBSSceneItemSelect(void *param, calldata_t *data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform *>(param);
	OBSSceneItem item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item != window->item)
		window->SetItem(item);
}

void OBSBasicTransform::OBSSceneItemDeselect(void *param, calldata_t *data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform *>(param);
	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(data, "scene");
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item == window->item) {
		window->setWindowTitle(QTStr("Basic.TransformWindow.NoSelectedSource"));
		window->SetItem(FindASelectedItem(scene));
	}
}

void OBSBasicTransform::OBSSceneItemLocked(void *param, calldata_t *data)
{
	OBSBasicTransform *window = reinterpret_cast<OBSBasicTransform *>(param);
	bool locked = calldata_bool(data, "locked");

	QMetaObject::invokeMethod(window, "SetEnabled", Q_ARG(bool, !locked));
}

static const uint32_t listToAlign[] = {OBS_ALIGN_TOP | OBS_ALIGN_LEFT,
				       OBS_ALIGN_TOP,
				       OBS_ALIGN_TOP | OBS_ALIGN_RIGHT,
				       OBS_ALIGN_LEFT,
				       OBS_ALIGN_CENTER,
				       OBS_ALIGN_RIGHT,
				       OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT,
				       OBS_ALIGN_BOTTOM,
				       OBS_ALIGN_BOTTOM | OBS_ALIGN_RIGHT};

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

	obs_transform_info osi;
	obs_sceneitem_crop crop;
	obs_sceneitem_get_info2(item, &osi);
	obs_sceneitem_get_crop(item, &crop);

	obs_source_t *source = obs_sceneitem_get_source(item);
	uint32_t source_cx = obs_source_get_width(source);
	uint32_t source_cy = obs_source_get_height(source);
	float width = float(source_cx);
	float height = float(source_cy);

	int alignIndex = AlignToList(osi.alignment);
	int boundsAlignIndex = AlignToList(osi.bounds_alignment);

	ignoreItemChange = true;
	ui->positionX->setValue(osi.pos.x);
	ui->positionY->setValue(osi.pos.y);
	ui->rotation->setValue(osi.rot);
	ui->sizeX->setValue(osi.scale.x * width);
	ui->sizeY->setValue(osi.scale.y * height);
	ui->align->setCurrentIndex(alignIndex);

	bool valid_size = source_cx != 0 && source_cy != 0;
	ui->sizeX->setEnabled(valid_size);
	ui->sizeY->setEnabled(valid_size);

	ui->boundsType->setCurrentIndex(int(osi.bounds_type));
	ui->boundsAlign->setCurrentIndex(boundsAlignIndex);
	ui->boundsWidth->setValue(osi.bounds.x);
	ui->boundsHeight->setValue(osi.bounds.y);
	ui->cropToBounds->setChecked(osi.crop_to_bounds);

	ui->cropLeft->setValue(int(crop.left));
	ui->cropRight->setValue(int(crop.right));
	ui->cropTop->setValue(int(crop.top));
	ui->cropBottom->setValue(int(crop.bottom));
	ignoreItemChange = false;

	std::string name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.TransformWindow.Title").arg(name.c_str()));
}

void OBSBasicTransform::OnBoundsType(int index)
{
	if (index == -1)
		return;

	obs_bounds_type type = (obs_bounds_type)index;
	bool enable = (type != OBS_BOUNDS_NONE);

	ui->boundsAlign->setEnabled(enable);
	ui->boundsWidth->setEnabled(enable);
	ui->boundsHeight->setEnabled(enable);
	ui->cropToBounds->setEnabled(enable);

	if (!ignoreItemChange) {
		obs_bounds_type lastType = obs_sceneitem_get_bounds_type(item);
		if (lastType == OBS_BOUNDS_NONE) {
			OBSSource source = obs_sceneitem_get_source(item);
			int width = (int)obs_source_get_width(source);
			int height = (int)obs_source_get_height(source);

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

	obs_source_t *source = obs_sceneitem_get_source(item);
	uint32_t source_cx = obs_source_get_width(source);
	uint32_t source_cy = obs_source_get_height(source);
	double width = double(source_cx);
	double height = double(source_cy);

	obs_transform_info oti;
	obs_sceneitem_get_info2(item, &oti);

	/* do not scale a source if it has 0 width/height */
	if (source_cx != 0 && source_cy != 0) {
		oti.scale.x = float(ui->sizeX->value() / width);
		oti.scale.y = float(ui->sizeY->value() / height);
	}

	oti.pos.x = float(ui->positionX->value());
	oti.pos.y = float(ui->positionY->value());
	oti.rot = float(ui->rotation->value());
	oti.alignment = listToAlign[ui->align->currentIndex()];

	oti.bounds_type = (obs_bounds_type)ui->boundsType->currentIndex();
	oti.bounds_alignment = listToAlign[ui->boundsAlign->currentIndex()];
	oti.bounds.x = float(ui->boundsWidth->value());
	oti.bounds.y = float(ui->boundsHeight->value());
	oti.crop_to_bounds = ui->cropToBounds->isChecked();

	ignoreTransformSignal = true;
	obs_sceneitem_set_info2(item, &oti);
	ignoreTransformSignal = false;
}

void OBSBasicTransform::OnCropChanged()
{
	if (ignoreItemChange)
		return;

	obs_sceneitem_crop crop;
	crop.left = uint32_t(ui->cropLeft->value());
	crop.right = uint32_t(ui->cropRight->value());
	crop.top = uint32_t(ui->cropTop->value());
	crop.bottom = uint32_t(ui->cropBottom->value());

	ignoreTransformSignal = true;
	obs_sceneitem_set_crop(item, &crop);
	ignoreTransformSignal = false;
}

void OBSBasicTransform::OnSceneChanged(QListWidgetItem *current, QListWidgetItem *)
{
	if (!current)
		return;

	OBSScene scene = GetOBSRef<OBSScene>(current);
	this->SetScene(scene);
}
