#include "OBSBasicTransform.hpp"

#include <widgets/OBSBasic.hpp>

#include "moc_OBSBasicTransform.cpp"

namespace {
static bool find_sel(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	OBSSceneItem &dst = *static_cast<OBSSceneItem *>(param);

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
static vec2 getAlignmentConversion(uint32_t alignment)
{
	vec2 ratio = {0.5f, 0.5f};
	if (alignment & OBS_ALIGN_RIGHT) {
		ratio.x = 1.0f;
	}
	if (alignment & OBS_ALIGN_LEFT) {
		ratio.x = 0.0f;
	}
	if (alignment & OBS_ALIGN_BOTTOM) {
		ratio.y = 1.0f;
	}
	if (alignment & OBS_ALIGN_TOP) {
		ratio.y = 0.0f;
	}

	return ratio;
}
} // namespace

#define COMBO_CHANGED &QComboBox::currentIndexChanged
#define ALIGN_CHANGED &AlignmentSelector::currentIndexChanged
#define ISCROLL_CHANGED &QSpinBox::valueChanged
#define DSCROLL_CHANGED &QDoubleSpinBox::valueChanged

OBSBasicTransform::OBSBasicTransform(OBSSceneItem item, OBSBasic *parent)
	: QDialog(parent),
	  ui(new Ui::OBSBasicTransform),
	  main(parent)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	positionAlignment = new AlignmentSelector(this);
	positionAlignment->setAccessibleName(QTStr("Basic.TransformWindow.Alignment"));
	ui->verticalLayout_4->addWidget(positionAlignment);
	positionAlignment->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	boundsAlignment = new AlignmentSelector(this);
	boundsAlignment->setAccessibleName(QTStr("Basic.TransformWindow.BoundsAlignment"));
	boundsAlignment->setEnabled(false);
	ui->verticalLayout_5->addWidget(boundsAlignment);
	boundsAlignment->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	/* Avoid needing to include colons in strings themselves by adding them here */
	ui->positionXLabel->setText(ui->positionXLabel->text() + ":");
	ui->positionYLabel->setText(ui->positionYLabel->text() + ":");
	ui->sizeWidthLabel->setText(ui->sizeWidthLabel->text() + ":");
	ui->sizeHeightLabel->setText(ui->sizeHeightLabel->text() + ":");
	ui->boundsWidthLabel->setText(ui->boundsWidthLabel->text() + ":");
	ui->boundsHeightLabel->setText(ui->boundsHeightLabel->text() + ":");
	ui->cropLeftLabel->setText(ui->cropLeftLabel->text() + ":");
	ui->cropTopLabel->setText(ui->cropTopLabel->text() + ":");
	ui->cropRightLabel->setText(ui->cropRightLabel->text() + ":");
	ui->cropBottomLabel->setText(ui->cropBottomLabel->text() + ":");

	setTabOrder(ui->rotation, positionAlignment);
	setTabOrder(ui->cropToBounds, boundsAlignment);

	hookWidget(ui->positionX, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->positionY, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->rotation, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->sizeX, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->sizeY, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(positionAlignment.get(), ALIGN_CHANGED, &OBSBasicTransform::onAlignChanged);
	hookWidget(ui->boundsType, COMBO_CHANGED, &OBSBasicTransform::onBoundsType);
	hookWidget(boundsAlignment.get(), ALIGN_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->boundsWidth, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->boundsHeight, DSCROLL_CHANGED, &OBSBasicTransform::onControlChanged);
	hookWidget(ui->cropLeft, ISCROLL_CHANGED, &OBSBasicTransform::onCropChanged);
	hookWidget(ui->cropRight, ISCROLL_CHANGED, &OBSBasicTransform::onCropChanged);
	hookWidget(ui->cropTop, ISCROLL_CHANGED, &OBSBasicTransform::onCropChanged);
	hookWidget(ui->cropBottom, ISCROLL_CHANGED, &OBSBasicTransform::onCropChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	hookWidget(ui->cropToBounds, &QCheckBox::checkStateChanged, &OBSBasicTransform::onControlChanged);
#else
	hookWidget(ui->cropToBounds, &QCheckBox::stateChanged, &OBSBasicTransform::onControlChanged);
#endif
	ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

	connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, main,
		&OBSBasic::on_actionResetTransform_triggered);

	installEventFilter(CreateShortcutFilter());

	OBSScene scene = obs_sceneitem_get_scene(item);
	setScene(scene);
	setItem(item);

	std::string name = obs_source_get_name(obs_sceneitem_get_source(item));
	setWindowTitle(QTStr("Basic.TransformWindow.Title").arg(name.c_str()));

	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(main->GetCurrentScene(), false);
	undo_data = std::string(obs_data_get_json(wrapper));

	adjustSize();
	setMinimumSize(size());
	setMaximumSize(size());
}

OBSBasicTransform::~OBSBasicTransform()
{
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(main->GetCurrentScene(), false);

	auto undo_redo = [](const std::string &data) {
		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
		OBSBasic::Get()->SetCurrentScene(source.Get(), true);
		obs_scene_load_transform_states(data.c_str());
	};

	std::string redo_data(obs_data_get_json(wrapper));
	if (undo_data.compare(redo_data) != 0)
		main->undo_s.add_action(
			QTStr("Undo.Transform").arg(obs_source_get_name(obs_scene_get_source(main->GetCurrentScene()))),
			undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasicTransform::setScene(OBSScene scene)
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

void OBSBasicTransform::setItem(OBSSceneItem newItem)
{
	QMetaObject::invokeMethod(this, "setItemQt", Q_ARG(OBSSceneItem, OBSSceneItem(newItem)));
}

void OBSBasicTransform::setEnabled(bool enable)
{
	ui->container->setEnabled(enable);
	ui->container2->setEnabled(enable);
	ui->container3->setEnabled(enable);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(enable);
}

void OBSBasicTransform::setItemQt(OBSSceneItem newItem)
{
	item = newItem;
	if (item)
		refreshControls();

	bool enable = !!item && !obs_sceneitem_locked(item);
	setEnabled(enable);
}

void OBSBasicTransform::OBSSceneItemTransform(void *param, calldata_t *data)
{
	OBSBasicTransform *window = static_cast<OBSBasicTransform *>(param);
	OBSSceneItem item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item == window->item && !window->ignoreTransformSignal)
		QMetaObject::invokeMethod(window, "refreshControls");
}

void OBSBasicTransform::OBSSceneItemRemoved(void *param, calldata_t *data)
{
	OBSBasicTransform *window = static_cast<OBSBasicTransform *>(param);
	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(data, "scene");
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item == window->item)
		window->setItem(FindASelectedItem(scene));
}

void OBSBasicTransform::OBSSceneItemSelect(void *param, calldata_t *data)
{
	OBSBasicTransform *window = static_cast<OBSBasicTransform *>(param);
	OBSSceneItem item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item != window->item)
		window->setItem(item);
}

void OBSBasicTransform::OBSSceneItemDeselect(void *param, calldata_t *data)
{
	OBSBasicTransform *window = static_cast<OBSBasicTransform *>(param);
	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(data, "scene");
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(data, "item");

	if (item == window->item) {
		window->setWindowTitle(QTStr("Basic.TransformWindow.NoSelectedSource"));
		window->setItem(FindASelectedItem(scene));
	}
}

void OBSBasicTransform::OBSSceneItemLocked(void *param, calldata_t *data)
{
	OBSBasicTransform *window = static_cast<OBSBasicTransform *>(param);
	bool locked = calldata_bool(data, "locked");

	QMetaObject::invokeMethod(window, "setEnabled", Q_ARG(bool, !locked));
}

static const uint32_t indexToAlign[] = {OBS_ALIGN_TOP | OBS_ALIGN_LEFT,
					OBS_ALIGN_TOP,
					OBS_ALIGN_TOP | OBS_ALIGN_RIGHT,
					OBS_ALIGN_LEFT,
					OBS_ALIGN_CENTER,
					OBS_ALIGN_RIGHT,
					OBS_ALIGN_BOTTOM | OBS_ALIGN_LEFT,
					OBS_ALIGN_BOTTOM,
					OBS_ALIGN_BOTTOM | OBS_ALIGN_RIGHT};

static int alignToIndex(uint32_t align)
{
	int index = 0;
	for (uint32_t curAlign : indexToAlign) {
		if (curAlign == align)
			return index;

		index++;
	}

	return 0;
}

void OBSBasicTransform::refreshControls()
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

	int alignIndex = alignToIndex(osi.alignment);
	int boundsAlignIndex = alignToIndex(osi.bounds_alignment);

	ignoreItemChange = true;
	ui->positionX->setValue(osi.pos.x);
	ui->positionY->setValue(osi.pos.y);
	ui->rotation->setValue(osi.rot);
	ui->sizeX->setValue(osi.scale.x * width);
	ui->sizeY->setValue(osi.scale.y * height);
	positionAlignment->setCurrentIndex(alignIndex);

	bool valid_size = source_cx != 0 && source_cy != 0;
	ui->sizeX->setEnabled(valid_size);
	ui->sizeY->setEnabled(valid_size);

	ui->boundsType->setCurrentIndex(int(osi.bounds_type));
	boundsAlignment->setCurrentIndex(boundsAlignIndex);
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

void OBSBasicTransform::onAlignChanged(int index)
{
	uint32_t alignment = indexToAlign[index];

	vec2 flipRatio = getAlignmentConversion(alignment);

	obs_transform_info osi;
	obs_sceneitem_crop crop;
	obs_sceneitem_get_info2(item, &osi);
	obs_sceneitem_get_crop(item, &crop);

	obs_source_t *source = obs_sceneitem_get_source(item);
	uint32_t sourceWidth = obs_source_get_width(source);
	uint32_t sourceHeight = obs_source_get_height(source);

	uint32_t widthForFlip = sourceWidth - crop.left - crop.right;
	uint32_t heightForFlip = sourceHeight - crop.top - crop.bottom;

	if (osi.bounds_type != OBS_BOUNDS_NONE) {
		widthForFlip = osi.bounds.x;
		heightForFlip = osi.bounds.y;
	}

	vec2 currentRatio = getAlignmentConversion(osi.alignment);

	float shiftX = (currentRatio.x - flipRatio.x) * widthForFlip * osi.scale.x;
	float shiftY = (currentRatio.y - flipRatio.y) * heightForFlip * osi.scale.y;

	bool previousIgnoreState = ignoreItemChange;

	ignoreItemChange = true;
	ui->positionX->setValue(osi.pos.x - shiftX);
	ui->positionY->setValue(osi.pos.y - shiftY);
	ignoreItemChange = previousIgnoreState;

	onControlChanged();
}

void OBSBasicTransform::onBoundsType(int index)
{
	if (index == -1)
		return;

	obs_bounds_type type = (obs_bounds_type)index;
	bool enable = (type != OBS_BOUNDS_NONE);

	boundsAlignment->setEnabled(enable && type != OBS_BOUNDS_STRETCH);
	ui->boundsWidth->setEnabled(enable);
	ui->boundsHeight->setEnabled(enable);

	bool isCoverBounds = type == OBS_BOUNDS_SCALE_OUTER || type == OBS_BOUNDS_SCALE_TO_WIDTH ||
			     type == OBS_BOUNDS_SCALE_TO_HEIGHT;
	ui->cropToBounds->setEnabled(isCoverBounds);

	if (!ignoreItemChange) {
		obs_bounds_type lastType = obs_sceneitem_get_bounds_type(item);
		if (lastType == OBS_BOUNDS_NONE) {
			OBSSource source = obs_sceneitem_get_source(item);
			int width = (int)obs_source_get_width(source);
			int height = (int)obs_source_get_height(source);

			vec2 scale;
			obs_sceneitem_get_scale(item, &scale);

			obs_sceneitem_crop crop;
			obs_sceneitem_get_crop(item, &crop);

			ui->sizeX->setValue(width);
			ui->sizeY->setValue(height);

			ui->boundsWidth->setValue((width - crop.left - crop.right) * scale.x);
			ui->boundsHeight->setValue((height - crop.top - crop.bottom) * scale.y);
		} else if (type == OBS_BOUNDS_NONE) {
			OBSSource source = obs_sceneitem_get_source(item);
			int width = (int)obs_source_get_width(source);
			int height = (int)obs_source_get_height(source);

			matrix4 draw;
			obs_sceneitem_get_draw_transform(item, &draw);

			ui->sizeX->setValue(width * draw.x.x);
			ui->sizeY->setValue(height * draw.y.y);

			obs_transform_info osi;
			obs_sceneitem_get_info2(item, &osi);

			/* We use the draw transform values here which is always a top left coordinate origin */
			vec2 currentRatio = getAlignmentConversion(OBS_ALIGN_TOP | OBS_ALIGN_LEFT);
			vec2 flipRatio = getAlignmentConversion(osi.alignment);

			float drawX = draw.t.x;
			float drawY = draw.t.y;

			obs_sceneitem_crop crop;
			obs_sceneitem_get_crop(item, &crop);

			uint32_t widthForFlip = width - crop.left - crop.right;
			uint32_t heightForFlip = height - crop.top - crop.bottom;

			float shiftX = (currentRatio.x - flipRatio.x) * (widthForFlip * draw.x.x);
			float shiftY = (currentRatio.y - flipRatio.y) * (heightForFlip * draw.y.y);

			ui->positionX->setValue(osi.pos.x - (osi.pos.x - drawX) - shiftX);
			ui->positionY->setValue(osi.pos.y - (osi.pos.y - drawY) - shiftY);
		}
	}

	onControlChanged();
}

void OBSBasicTransform::onControlChanged()
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
	oti.alignment = indexToAlign[positionAlignment->currentIndex()];

	oti.bounds_type = (obs_bounds_type)ui->boundsType->currentIndex();
	oti.bounds_alignment = indexToAlign[boundsAlignment->currentIndex()];
	oti.bounds.x = float(ui->boundsWidth->value());
	oti.bounds.y = float(ui->boundsHeight->value());
	oti.crop_to_bounds = ui->cropToBounds->isChecked();

	ignoreTransformSignal = true;
	obs_sceneitem_set_info2(item, &oti);
	ignoreTransformSignal = false;
}

void OBSBasicTransform::onCropChanged()
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

void OBSBasicTransform::onSceneChanged(QListWidgetItem *current, QListWidgetItem *)
{
	if (!current)
		return;

	OBSScene scene = GetOBSRef<OBSScene>(current);
	this->setScene(scene);
}
