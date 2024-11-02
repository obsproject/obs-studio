#include <QGuiApplication>
#include <QMouseEvent>

#include <cmath>
#include <string>
#include <graphics/vec4.h>
#include <graphics/matrix4.h>
#include <util/dstr.hpp>
#include "moc_window-basic-preview.cpp"
#include "window-basic-main.hpp"
#include "obs-app.hpp"
#include "platform.hpp"
#include "display-helpers.hpp"

#define HANDLE_RADIUS 4.0f
#define HANDLE_SEL_RADIUS (HANDLE_RADIUS * 1.5f)
#define HELPER_ROT_BREAKPOINT 45.0f

/* TODO: make C++ math classes and clean up code here later */

OBSBasicPreview::OBSBasicPreview(QWidget *parent, Qt::WindowFlags flags) : OBSQTDisplay(parent, flags)
{
	ResetScrollingOffset();
	setMouseTracking(true);
}

OBSBasicPreview::~OBSBasicPreview()
{
	obs_enter_graphics();

	if (overflow)
		gs_texture_destroy(overflow);
	if (rectFill)
		gs_vertexbuffer_destroy(rectFill);
	if (circleFill)
		gs_vertexbuffer_destroy(circleFill);

	obs_leave_graphics();
}

void OBSBasicPreview::Init()
{
	OBSBasic *main = OBSBasic::Get();
	connect(main, &OBSBasic::PreviewXScrollBarMoved, this, &OBSBasicPreview::XScrollBarMoved);
	connect(main, &OBSBasic::PreviewYScrollBarMoved, this, &OBSBasicPreview::YScrollBarMoved);
}

vec2 OBSBasicPreview::GetMouseEventPos(QMouseEvent *event)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	float pixelRatio = main->GetDevicePixelRatio();
	float scale = pixelRatio / main->previewScale;
	QPoint qtPos = event->pos();
	vec2 pos;
	vec2_set(&pos, (qtPos.x() - main->previewX / pixelRatio) * scale,
		 (qtPos.y() - main->previewY / pixelRatio) * scale);

	return pos;
}

static void RotatePos(vec2 *pos, float rot)
{
	float cosR = cos(rot);
	float sinR = sin(rot);

	vec2 newPos;

	newPos.x = cosR * pos->x - sinR * pos->y;
	newPos.y = sinR * pos->x + cosR * pos->y;

	vec2_copy(pos, &newPos);
}

struct SceneFindData {
	const vec2 &pos;
	OBSSceneItem item;
	bool selectBelow;

	obs_sceneitem_t *group = nullptr;

	SceneFindData(const SceneFindData &) = delete;
	SceneFindData(SceneFindData &&) = delete;
	SceneFindData &operator=(const SceneFindData &) = delete;
	SceneFindData &operator=(SceneFindData &&) = delete;

	inline SceneFindData(const vec2 &pos_, bool selectBelow_) : pos(pos_), selectBelow(selectBelow_) {}
};

struct SceneFindBoxData {
	const vec2 &startPos;
	const vec2 &pos;
	std::vector<obs_sceneitem_t *> sceneItems;

	SceneFindBoxData(const SceneFindData &) = delete;
	SceneFindBoxData(SceneFindData &&) = delete;
	SceneFindBoxData &operator=(const SceneFindData &) = delete;
	SceneFindBoxData &operator=(SceneFindData &&) = delete;

	inline SceneFindBoxData(const vec2 &startPos_, const vec2 &pos_) : startPos(startPos_), pos(pos_) {}
};

static bool SceneItemHasVideo(obs_sceneitem_t *item)
{
	obs_source_t *source = obs_sceneitem_get_source(item);
	uint32_t flags = obs_source_get_output_flags(source);
	return (flags & OBS_SOURCE_VIDEO) != 0;
}

static bool CloseFloat(float a, float b, float epsilon = 0.01)
{
	using std::abs;
	return abs(a - b) <= epsilon;
}

static bool FindItemAtPos(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	SceneFindData *data = reinterpret_cast<SceneFindData *>(param);
	matrix4 transform;
	matrix4 invTransform;
	vec3 transformedPos;
	vec3 pos3;
	vec3 pos3_;

	if (!SceneItemHasVideo(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	matrix4_inv(&invTransform, &transform);
	vec3_transform(&transformedPos, &pos3, &invTransform);
	vec3_transform(&pos3_, &transformedPos, &transform);

	if (CloseFloat(pos3.x, pos3_.x) && CloseFloat(pos3.y, pos3_.y) && transformedPos.x >= 0.0f &&
	    transformedPos.x <= 1.0f && transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {
		if (data->selectBelow && obs_sceneitem_selected(item)) {
			if (data->item)
				return false;
			else
				data->selectBelow = false;
		}

		data->item = item;
	}

	return true;
}

static vec3 GetTransformedPos(float x, float y, const matrix4 &mat)
{
	vec3 result;
	vec3_set(&result, x, y, 0.0f);
	vec3_transform(&result, &result, &mat);
	return result;
}

static inline vec2 GetOBSScreenSize()
{
	obs_video_info ovi;
	vec2 size;
	vec2_zero(&size);

	if (obs_get_video_info(&ovi)) {
		size.x = float(ovi.base_width);
		size.y = float(ovi.base_height);
	}

	return size;
}

vec3 OBSBasicPreview::GetSnapOffset(const vec3 &tl, const vec3 &br)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	vec2 screenSize = GetOBSScreenSize();
	vec3 clampOffset;

	vec3_zero(&clampOffset);

	const bool snap = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SnappingEnabled");
	if (snap == false)
		return clampOffset;

	const bool screenSnap = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ScreenSnapping");
	const bool centerSnap = config_get_bool(App()->GetUserConfig(), "BasicWindow", "CenterSnapping");

	const float clampDist =
		config_get_double(App()->GetUserConfig(), "BasicWindow", "SnapDistance") / main->previewScale;
	const float centerX = br.x - (br.x - tl.x) / 2.0f;
	const float centerY = br.y - (br.y - tl.y) / 2.0f;

	// Left screen edge.
	if (screenSnap && fabsf(tl.x) < clampDist)
		clampOffset.x = -tl.x;
	// Right screen edge.
	if (screenSnap && fabsf(clampOffset.x) < EPSILON && fabsf(screenSize.x - br.x) < clampDist)
		clampOffset.x = screenSize.x - br.x;
	// Horizontal center.
	if (centerSnap && fabsf(screenSize.x - (br.x - tl.x)) > clampDist &&
	    fabsf(screenSize.x / 2.0f - centerX) < clampDist)
		clampOffset.x = screenSize.x / 2.0f - centerX;

	// Top screen edge.
	if (screenSnap && fabsf(tl.y) < clampDist)
		clampOffset.y = -tl.y;
	// Bottom screen edge.
	if (screenSnap && fabsf(clampOffset.y) < EPSILON && fabsf(screenSize.y - br.y) < clampDist)
		clampOffset.y = screenSize.y - br.y;
	// Vertical center.
	if (centerSnap && fabsf(screenSize.y - (br.y - tl.y)) > clampDist &&
	    fabsf(screenSize.y / 2.0f - centerY) < clampDist)
		clampOffset.y = screenSize.y / 2.0f - centerY;

	return clampOffset;
}

OBSSceneItem OBSBasicPreview::GetItemAtPos(const vec2 &pos, bool selectBelow)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSScene scene = main->GetCurrentScene();
	if (!scene)
		return OBSSceneItem();

	SceneFindData data(pos, selectBelow);
	obs_scene_enum_items(scene, FindItemAtPos, &data);
	return data.item;
}

static bool CheckItemSelected(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	SceneFindData *data = reinterpret_cast<SceneFindData *>(param);
	matrix4 transform;
	vec3 transformedPos;
	vec3 pos3;

	if (!SceneItemHasVideo(item))
		return true;
	if (obs_sceneitem_is_group(item)) {
		data->group = item;
		obs_sceneitem_group_enum_items(item, CheckItemSelected, param);
		data->group = nullptr;

		if (data->item) {
			return false;
		}
	}

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	if (data->group) {
		matrix4 parent_transform;
		obs_sceneitem_get_draw_transform(data->group, &parent_transform);
		matrix4_mul(&transform, &transform, &parent_transform);
	}

	matrix4_inv(&transform, &transform);
	vec3_transform(&transformedPos, &pos3, &transform);

	if (transformedPos.x >= 0.0f && transformedPos.x <= 1.0f && transformedPos.y >= 0.0f &&
	    transformedPos.y <= 1.0f) {
		if (obs_sceneitem_selected(item)) {
			data->item = item;
			return false;
		}
	}

	return true;
}

bool OBSBasicPreview::SelectedAtPos(const vec2 &pos)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSScene scene = main->GetCurrentScene();
	if (!scene)
		return false;

	SceneFindData data(pos, false);
	obs_scene_enum_items(scene, CheckItemSelected, &data);
	return !!data.item;
}

struct HandleFindData {
	const vec2 &pos;
	const float radius;
	matrix4 parent_xform;

	OBSSceneItem item;
	ItemHandle handle = ItemHandle::None;
	float angle = 0.0f;
	vec2 rotatePoint;
	vec2 offsetPoint;

	float angleOffset = 0.0f;

	HandleFindData(const HandleFindData &) = delete;
	HandleFindData(HandleFindData &&) = delete;
	HandleFindData &operator=(const HandleFindData &) = delete;
	HandleFindData &operator=(HandleFindData &&) = delete;

	inline HandleFindData(const vec2 &pos_, float scale) : pos(pos_), radius(HANDLE_SEL_RADIUS / scale)
	{
		matrix4_identity(&parent_xform);
	}

	inline HandleFindData(const HandleFindData &hfd, obs_sceneitem_t *parent)
		: pos(hfd.pos),
		  radius(hfd.radius),
		  item(hfd.item),
		  handle(hfd.handle),
		  angle(hfd.angle),
		  rotatePoint(hfd.rotatePoint),
		  offsetPoint(hfd.offsetPoint)
	{
		obs_sceneitem_get_draw_transform(parent, &parent_xform);
	}
};

static bool FindHandleAtPos(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	HandleFindData &data = *reinterpret_cast<HandleFindData *>(param);

	if (!obs_sceneitem_selected(item)) {
		if (obs_sceneitem_is_group(item)) {
			HandleFindData newData(data, item);
			newData.angleOffset = obs_sceneitem_get_rot(item);

			obs_sceneitem_group_enum_items(item, FindHandleAtPos, &newData);

			data.item = newData.item;
			data.handle = newData.handle;
			data.angle = newData.angle;
			data.rotatePoint = newData.rotatePoint;
			data.offsetPoint = newData.offsetPoint;
		}

		return true;
	}

	matrix4 transform;
	vec3 pos3;
	float closestHandle = data.radius;

	vec3_set(&pos3, data.pos.x, data.pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	auto TestHandle = [&](float x, float y, ItemHandle handle) {
		vec3 handlePos = GetTransformedPos(x, y, transform);
		vec3_transform(&handlePos, &handlePos, &data.parent_xform);

		float dist = vec3_dist(&handlePos, &pos3);
		if (dist < data.radius) {
			if (dist < closestHandle) {
				closestHandle = dist;
				data.handle = handle;
				data.item = item;
			}
		}
	};

	TestHandle(0.0f, 0.0f, ItemHandle::TopLeft);
	TestHandle(0.5f, 0.0f, ItemHandle::TopCenter);
	TestHandle(1.0f, 0.0f, ItemHandle::TopRight);
	TestHandle(0.0f, 0.5f, ItemHandle::CenterLeft);
	TestHandle(1.0f, 0.5f, ItemHandle::CenterRight);
	TestHandle(0.0f, 1.0f, ItemHandle::BottomLeft);
	TestHandle(0.5f, 1.0f, ItemHandle::BottomCenter);
	TestHandle(1.0f, 1.0f, ItemHandle::BottomRight);

	vec2 scale;
	obs_sceneitem_get_scale(item, &scale);
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(item);
	vec2 rotHandleOffset;
	vec2_set(&rotHandleOffset, 0.0f, HANDLE_RADIUS * data.radius * 1.5 - data.radius);
	bool invertx = scale.x < 0.0f && boundsType == OBS_BOUNDS_NONE;
	float angle =
		atan2(invertx ? transform.x.y * -1.0f : transform.x.y, invertx ? transform.x.x * -1.0f : transform.x.x);
	RotatePos(&rotHandleOffset, angle);
	RotatePos(&rotHandleOffset, RAD(data.angleOffset));

	bool inverty = scale.y < 0.0f && boundsType == OBS_BOUNDS_NONE;
	vec3 handlePos = GetTransformedPos(0.5f, inverty ? 1.0f : 0.0f, transform);
	vec3_transform(&handlePos, &handlePos, &data.parent_xform);
	handlePos.x -= rotHandleOffset.x;
	handlePos.y -= rotHandleOffset.y;

	float dist = vec3_dist(&handlePos, &pos3);
	if (dist < data.radius) {
		if (dist < closestHandle) {
			closestHandle = dist;
			data.item = item;
			data.angle = obs_sceneitem_get_rot(item);
			data.handle = ItemHandle::Rot;

			vec2_set(&data.rotatePoint, transform.t.x + transform.x.x / 2 + transform.y.x / 2,
				 transform.t.y + transform.x.y / 2 + transform.y.y / 2);

			obs_sceneitem_get_pos(item, &data.offsetPoint);
			data.offsetPoint.x -= data.rotatePoint.x;
			data.offsetPoint.y -= data.rotatePoint.y;

			RotatePos(&data.offsetPoint, -RAD(obs_sceneitem_get_rot(item)));
		}
	}

	return true;
}

static vec2 GetItemSize(obs_sceneitem_t *item)
{
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(item);
	vec2 size;

	if (boundsType != OBS_BOUNDS_NONE) {
		obs_sceneitem_get_bounds(item, &size);
	} else {
		obs_source_t *source = obs_sceneitem_get_source(item);
		obs_sceneitem_crop crop;
		vec2 scale;

		obs_sceneitem_get_scale(item, &scale);
		obs_sceneitem_get_crop(item, &crop);
		size.x = float(obs_source_get_width(source) - crop.left - crop.right) * scale.x;
		size.y = float(obs_source_get_height(source) - crop.top - crop.bottom) * scale.y;
	}

	return size;
}

void OBSBasicPreview::GetStretchHandleData(const vec2 &pos, bool ignoreGroup)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSScene scene = main->GetCurrentScene();
	if (!scene)
		return;

	float scale = main->previewScale / main->GetDevicePixelRatio();
	vec2 scaled_pos = pos;
	vec2_divf(&scaled_pos, &scaled_pos, scale);
	HandleFindData data(scaled_pos, scale);
	obs_scene_enum_items(scene, FindHandleAtPos, &data);

	stretchItem = std::move(data.item);
	stretchHandle = data.handle;

	rotateAngle = data.angle;
	rotatePoint = data.rotatePoint;
	offsetPoint = data.offsetPoint;

	if (stretchHandle != ItemHandle::None) {
		matrix4 boxTransform;
		vec3 itemUL;
		float itemRot;

		stretchItemSize = GetItemSize(stretchItem);

		obs_sceneitem_get_box_transform(stretchItem, &boxTransform);
		itemRot = obs_sceneitem_get_rot(stretchItem);
		vec3_from_vec4(&itemUL, &boxTransform.t);

		/* build the item space conversion matrices */
		matrix4_identity(&itemToScreen);
		matrix4_rotate_aa4f(&itemToScreen, &itemToScreen, 0.0f, 0.0f, 1.0f, RAD(itemRot));
		matrix4_translate3f(&itemToScreen, &itemToScreen, itemUL.x, itemUL.y, 0.0f);

		matrix4_identity(&screenToItem);
		matrix4_translate3f(&screenToItem, &screenToItem, -itemUL.x, -itemUL.y, 0.0f);
		matrix4_rotate_aa4f(&screenToItem, &screenToItem, 0.0f, 0.0f, 1.0f, RAD(-itemRot));

		obs_sceneitem_get_crop(stretchItem, &startCrop);
		obs_sceneitem_get_pos(stretchItem, &startItemPos);

		obs_source_t *source = obs_sceneitem_get_source(stretchItem);
		cropSize.x = float(obs_source_get_width(source) - startCrop.left - startCrop.right);
		cropSize.y = float(obs_source_get_height(source) - startCrop.top - startCrop.bottom);

		stretchGroup = obs_sceneitem_get_group(scene, stretchItem);
		if (stretchGroup && !ignoreGroup) {
			obs_sceneitem_get_draw_transform(stretchGroup, &invGroupTransform);
			matrix4_inv(&invGroupTransform, &invGroupTransform);
			obs_sceneitem_defer_group_resize_begin(stretchGroup);
		} else {
			stretchGroup = nullptr;
		}
	}
}

void OBSBasicPreview::keyPressEvent(QKeyEvent *event)
{
	if (!IsFixedScaling() || event->isAutoRepeat()) {
		OBSQTDisplay::keyPressEvent(event);
		return;
	}

	switch (event->key()) {
	case Qt::Key_Space:
		setCursor(Qt::OpenHandCursor);
		scrollMode = true;
		break;
	}

	OBSQTDisplay::keyPressEvent(event);
}

void OBSBasicPreview::keyReleaseEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat()) {
		OBSQTDisplay::keyReleaseEvent(event);
		return;
	}

	switch (event->key()) {
	case Qt::Key_Space:
		scrollMode = false;
		setCursor(Qt::ArrowCursor);
		break;
	}

	OBSQTDisplay::keyReleaseEvent(event);
}

void OBSBasicPreview::wheelEvent(QWheelEvent *event)
{
	if (scrollMode && IsFixedScaling()) {
		const int delta = event->angleDelta().y();
		if (delta != 0) {
			if (delta > 0)
				SetScalingLevel(scalingLevel + 1);
			else
				SetScalingLevel(scalingLevel - 1);
			emit DisplayResized();
		}
	}

	OBSQTDisplay::wheelEvent(event);
}

void OBSBasicPreview::mousePressEvent(QMouseEvent *event)
{
	QPointF pos = event->position();

	if (scrollMode && IsFixedScaling() && event->button() == Qt::LeftButton) {
		setCursor(Qt::ClosedHandCursor);
		scrollingFrom.x = pos.x();
		scrollingFrom.y = pos.y();
		return;
	}

	if (event->button() == Qt::RightButton) {
		scrollMode = false;
		setCursor(Qt::ArrowCursor);
	}

	if (locked) {
		OBSQTDisplay::mousePressEvent(event);
		return;
	}

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	float pixelRatio = main->GetDevicePixelRatio();
	float x = pos.x() - main->previewX / pixelRatio;
	float y = pos.y() - main->previewY / pixelRatio;
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	bool altDown = (modifiers & Qt::AltModifier);
	bool shiftDown = (modifiers & Qt::ShiftModifier);
	bool ctrlDown = (modifiers & Qt::ControlModifier);

	OBSQTDisplay::mousePressEvent(event);

	if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton)
		return;

	if (event->button() == Qt::LeftButton)
		mouseDown = true;

	{
		std::lock_guard<std::mutex> lock(selectMutex);
		selectedItems.clear();
	}

	if (altDown)
		cropping = true;

	if (altDown || shiftDown || ctrlDown) {
		vec2 s;
		SceneFindBoxData data(s, s);

		obs_scene_enum_items(main->GetCurrentScene(), FindSelected, &data);

		std::lock_guard<std::mutex> lock(selectMutex);
		selectedItems = data.sceneItems;
	}

	vec2_set(&startPos, x, y);
	GetStretchHandleData(startPos, false);

	vec2_divf(&startPos, &startPos, main->previewScale / pixelRatio);
	startPos.x = std::round(startPos.x);
	startPos.y = std::round(startPos.y);

	mouseOverItems = SelectedAtPos(startPos);
	vec2_zero(&lastMoveOffset);

	mousePos = startPos;
	wrapper = obs_scene_save_transform_states(main->GetCurrentScene(), true);
	changed = false;
}

void OBSBasicPreview::UpdateCursor(uint32_t &flags)
{
	if (!stretchItem || obs_sceneitem_locked(stretchItem)) {
		unsetCursor();
		return;
	}

	if (!flags && (cursor().shape() != Qt::OpenHandCursor || !scrollMode))
		unsetCursor();
	if ((cursor().shape() != Qt::ArrowCursor) || flags == 0)
		return;

	if (flags & ITEM_ROT) {
		setCursor(Qt::OpenHandCursor);
		return;
	}

	float rotation = obs_sceneitem_get_rot(stretchItem);
	vec2 scale;
	obs_sceneitem_get_scale(stretchItem, &scale);

	if (rotation < 0.0f)
		rotation = 360.0f + rotation;

	int octant = int(std::round(rotation / 45.0f));
	bool isCorner = (flags & (flags - 1)) != 0;

	if ((scale.x < 0.0f) && isCorner)
		flags ^= ITEM_LEFT | ITEM_RIGHT;
	if ((scale.y < 0.0f) && isCorner)
		flags ^= ITEM_TOP | ITEM_BOTTOM;

	if (octant % 4 >= 2) {
		if (isCorner) {
			flags ^= ITEM_TOP | ITEM_BOTTOM;
		} else {
			flags = (flags >> 2) | (flags << 2);
		}
	}

	if (octant % 2 == 1) {
		if (isCorner) {
			flags &= (flags % 3 == 0) ? ~ITEM_TOP & ~ITEM_BOTTOM : ~ITEM_LEFT & ~ITEM_RIGHT;
		} else {
			flags = (flags % 4 == 0) ? flags | flags >> ((flags / 2) - 1)
						 : flags | ((flags >> 2) | (flags << 2));
		}
	}

	if ((flags & ITEM_LEFT && flags & ITEM_TOP) || (flags & ITEM_RIGHT && flags & ITEM_BOTTOM))
		setCursor(Qt::SizeFDiagCursor);
	else if ((flags & ITEM_LEFT && flags & ITEM_BOTTOM) || (flags & ITEM_RIGHT && flags & ITEM_TOP))
		setCursor(Qt::SizeBDiagCursor);
	else if (flags & ITEM_LEFT || flags & ITEM_RIGHT)
		setCursor(Qt::SizeHorCursor);
	else if (flags & ITEM_TOP || flags & ITEM_BOTTOM)
		setCursor(Qt::SizeVerCursor);
}

static bool select_one(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	obs_sceneitem_t *selectedItem = reinterpret_cast<obs_sceneitem_t *>(param);
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, select_one, param);

	obs_sceneitem_select(item, (selectedItem == item));

	return true;
}

void OBSBasicPreview::DoSelect(const vec2 &pos)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSScene scene = main->GetCurrentScene();
	OBSSceneItem item = GetItemAtPos(pos, true);

	obs_scene_enum_items(scene, select_one, (obs_sceneitem_t *)item);
}

void OBSBasicPreview::DoCtrlSelect(const vec2 &pos)
{
	OBSSceneItem item = GetItemAtPos(pos, false);
	if (!item)
		return;

	bool selected = obs_sceneitem_selected(item);
	obs_sceneitem_select(item, !selected);
}

void OBSBasicPreview::ProcessClick(const vec2 &pos)
{
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

	if (modifiers & Qt::ControlModifier)
		DoCtrlSelect(pos);
	else
		DoSelect(pos);
}

void OBSBasicPreview::mouseReleaseEvent(QMouseEvent *event)
{
	if (scrollMode)
		setCursor(Qt::OpenHandCursor);

	if (locked) {
		OBSQTDisplay::mouseReleaseEvent(event);
		return;
	}

	if (mouseDown) {
		vec2 pos = GetMouseEventPos(event);

		if (!mouseMoved)
			ProcessClick(pos);

		if (selectionBox) {
			Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

			bool altDown = modifiers & Qt::AltModifier;
			bool shiftDown = modifiers & Qt::ShiftModifier;
			bool ctrlDown = modifiers & Qt::ControlModifier;

			std::lock_guard<std::mutex> lock(selectMutex);
			if (altDown || ctrlDown || shiftDown) {
				for (const auto &item : selectedItems) {
					obs_sceneitem_select(item, true);
				}
			}

			for (const auto &item : hoveredPreviewItems) {
				bool select = true;

				if (altDown) {
					select = false;
				} else if (ctrlDown) {
					select = !obs_sceneitem_selected(item);
				}

				obs_sceneitem_select(item, select);
			}
		}

		if (stretchGroup) {
			obs_sceneitem_defer_group_resize_end(stretchGroup);
		}

		stretchItem = nullptr;
		stretchGroup = nullptr;
		mouseDown = false;
		mouseMoved = false;
		cropping = false;
		selectionBox = false;
		unsetCursor();

		OBSSceneItem item = GetItemAtPos(pos, true);

		std::lock_guard<std::mutex> lock(selectMutex);
		hoveredPreviewItems.clear();
		hoveredPreviewItems.push_back(item);
		selectedItems.clear();
	}
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(main->GetCurrentScene(), true);

	auto undo_redo = [](const std::string &data) {
		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
		reinterpret_cast<OBSBasic *>(App()->GetMainWindow())->SetCurrentScene(source.Get(), true);

		obs_scene_load_transform_states(data.c_str());
	};

	if (wrapper && rwrapper) {
		std::string undo_data(obs_data_get_json(wrapper));
		std::string redo_data(obs_data_get_json(rwrapper));
		if (changed && undo_data.compare(redo_data) != 0)
			main->undo_s.add_action(
				QTStr("Undo.Transform").arg(obs_source_get_name(main->GetCurrentSceneSource())),
				undo_redo, undo_redo, undo_data, redo_data);
	}

	wrapper = nullptr;
}

struct SelectedItemBounds {
	bool first = true;
	vec3 tl, br;
};

static bool AddItemBounds(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	SelectedItemBounds *data = reinterpret_cast<SelectedItemBounds *>(param);
	vec3 t[4];

	auto add_bounds = [data, &t]() {
		for (const vec3 &v : t) {
			if (data->first) {
				vec3_copy(&data->tl, &v);
				vec3_copy(&data->br, &v);
				data->first = false;
			} else {
				vec3_min(&data->tl, &data->tl, &v);
				vec3_max(&data->br, &data->br, &v);
			}
		}
	};

	if (obs_sceneitem_is_group(item)) {
		SelectedItemBounds sib;
		obs_sceneitem_group_enum_items(item, AddItemBounds, &sib);

		if (!sib.first) {
			matrix4 xform;
			obs_sceneitem_get_draw_transform(item, &xform);

			vec3_set(&t[0], sib.tl.x, sib.tl.y, 0.0f);
			vec3_set(&t[1], sib.tl.x, sib.br.y, 0.0f);
			vec3_set(&t[2], sib.br.x, sib.tl.y, 0.0f);
			vec3_set(&t[3], sib.br.x, sib.br.y, 0.0f);
			vec3_transform(&t[0], &t[0], &xform);
			vec3_transform(&t[1], &t[1], &xform);
			vec3_transform(&t[2], &t[2], &xform);
			vec3_transform(&t[3], &t[3], &xform);
			add_bounds();
		}
	}
	if (!obs_sceneitem_selected(item))
		return true;

	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	t[0] = GetTransformedPos(0.0f, 0.0f, boxTransform);
	t[1] = GetTransformedPos(1.0f, 0.0f, boxTransform);
	t[2] = GetTransformedPos(0.0f, 1.0f, boxTransform);
	t[3] = GetTransformedPos(1.0f, 1.0f, boxTransform);
	add_bounds();

	return true;
}

struct OffsetData {
	float clampDist;
	vec3 tl, br, offset;
};

static bool GetSourceSnapOffset(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	OffsetData *data = reinterpret_cast<OffsetData *>(param);

	if (obs_sceneitem_selected(item))
		return true;

	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3 t[4] = {GetTransformedPos(0.0f, 0.0f, boxTransform), GetTransformedPos(1.0f, 0.0f, boxTransform),
		     GetTransformedPos(0.0f, 1.0f, boxTransform), GetTransformedPos(1.0f, 1.0f, boxTransform)};

	bool first = true;
	vec3 tl, br;
	vec3_zero(&tl);
	vec3_zero(&br);
	for (const vec3 &v : t) {
		if (first) {
			vec3_copy(&tl, &v);
			vec3_copy(&br, &v);
			first = false;
		} else {
			vec3_min(&tl, &tl, &v);
			vec3_max(&br, &br, &v);
		}
	}

	// Snap to other source edges
#define EDGE_SNAP(l, r, x, y)                                                                         \
	do {                                                                                          \
		double dist = fabsf(l.x - data->r.x);                                                 \
		if (dist < data->clampDist && fabsf(data->offset.x) < EPSILON && data->tl.y < br.y && \
		    data->br.y > tl.y && (fabsf(data->offset.x) > dist || data->offset.x < EPSILON))  \
			data->offset.x = l.x - data->r.x;                                             \
	} while (false)

	EDGE_SNAP(tl, br, x, y);
	EDGE_SNAP(tl, br, y, x);
	EDGE_SNAP(br, tl, x, y);
	EDGE_SNAP(br, tl, y, x);
#undef EDGE_SNAP

	return true;
}

void OBSBasicPreview::SnapItemMovement(vec2 &offset)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	OBSScene scene = main->GetCurrentScene();

	SelectedItemBounds data;
	obs_scene_enum_items(scene, AddItemBounds, &data);

	data.tl.x += offset.x;
	data.tl.y += offset.y;
	data.br.x += offset.x;
	data.br.y += offset.y;

	vec3 snapOffset = GetSnapOffset(data.tl, data.br);

	const bool snap = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SnappingEnabled");
	const bool sourcesSnap = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SourceSnapping");
	if (snap == false)
		return;
	if (sourcesSnap == false) {
		offset.x += snapOffset.x;
		offset.y += snapOffset.y;
		return;
	}

	const float clampDist =
		config_get_double(App()->GetUserConfig(), "BasicWindow", "SnapDistance") / main->previewScale;

	OffsetData offsetData;
	offsetData.clampDist = clampDist;
	offsetData.tl = data.tl;
	offsetData.br = data.br;
	vec3_copy(&offsetData.offset, &snapOffset);

	obs_scene_enum_items(scene, GetSourceSnapOffset, &offsetData);

	if (fabsf(offsetData.offset.x) > EPSILON || fabsf(offsetData.offset.y) > EPSILON) {
		offset.x += offsetData.offset.x;
		offset.y += offsetData.offset.y;
	} else {
		offset.x += snapOffset.x;
		offset.y += snapOffset.y;
	}
}

static bool move_items(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_locked(item))
		return true;

	bool selected = obs_sceneitem_selected(item);
	vec2 *offset = reinterpret_cast<vec2 *>(param);

	if (obs_sceneitem_is_group(item) && !selected) {
		matrix4 transform;
		vec3 new_offset;
		vec3_set(&new_offset, offset->x, offset->y, 0.0f);

		obs_sceneitem_get_draw_transform(item, &transform);
		vec4_set(&transform.t, 0.0f, 0.0f, 0.0f, 1.0f);
		matrix4_inv(&transform, &transform);
		vec3_transform(&new_offset, &new_offset, &transform);
		obs_sceneitem_group_enum_items(item, move_items, &new_offset);
	}

	if (selected) {
		vec2 pos;
		obs_sceneitem_get_pos(item, &pos);
		vec2_add(&pos, &pos, offset);
		obs_sceneitem_set_pos(item, &pos);
	}

	return true;
}

void OBSBasicPreview::MoveItems(const vec2 &pos)
{
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	OBSScene scene = main->GetCurrentScene();

	vec2 offset, moveOffset;
	vec2_sub(&offset, &pos, &startPos);
	vec2_sub(&moveOffset, &offset, &lastMoveOffset);

	if (!(modifiers & Qt::ControlModifier))
		SnapItemMovement(moveOffset);

	vec2_add(&lastMoveOffset, &lastMoveOffset, &moveOffset);

	obs_scene_enum_items(scene, move_items, &moveOffset);
}

static bool CounterClockwise(float x1, float x2, float x3, float y1, float y2, float y3)
{
	return (y3 - y1) * (x2 - x1) > (y2 - y1) * (x3 - x1);
}

static bool IntersectLine(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)
{
	bool a = CounterClockwise(x1, x2, x3, y1, y2, y3);
	bool b = CounterClockwise(x1, x2, x4, y1, y2, y4);
	bool c = CounterClockwise(x3, x4, x1, y3, y4, y1);
	bool d = CounterClockwise(x3, x4, x2, y3, y4, y2);

	return (a != b) && (c != d);
}

static bool IntersectBox(matrix4 transform, float x1, float x2, float y1, float y2)
{
	float x3, x4, y3, y4;

	x3 = transform.t.x;
	y3 = transform.t.y;
	x4 = x3 + transform.x.x;
	y4 = y3 + transform.x.y;

	if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
	    IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
		return true;

	x4 = x3 + transform.y.x;
	y4 = y3 + transform.y.y;

	if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
	    IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
		return true;

	x3 = transform.t.x + transform.x.x;
	y3 = transform.t.y + transform.x.y;
	x4 = x3 + transform.y.x;
	y4 = y3 + transform.y.y;

	if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
	    IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
		return true;

	x3 = transform.t.x + transform.y.x;
	y3 = transform.t.y + transform.y.y;
	x4 = x3 + transform.x.x;
	y4 = y3 + transform.x.y;

	if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
	    IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) || IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
		return true;

	return false;
}
#undef PI

bool OBSBasicPreview::FindSelected(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	SceneFindBoxData *data = reinterpret_cast<SceneFindBoxData *>(param);

	if (obs_sceneitem_selected(item))
		data->sceneItems.push_back(item);

	return true;
}

static bool FindItemsInBox(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	SceneFindBoxData *data = reinterpret_cast<SceneFindBoxData *>(param);
	matrix4 transform;
	matrix4 invTransform;
	vec3 transformedPos;
	vec3 pos3;
	vec3 pos3_;

	vec2 pos_min, pos_max;
	vec2_min(&pos_min, &data->startPos, &data->pos);
	vec2_max(&pos_max, &data->startPos, &data->pos);

	const float x1 = pos_min.x;
	const float x2 = pos_max.x;
	const float y1 = pos_min.y;
	const float y2 = pos_max.y;

	if (!SceneItemHasVideo(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;
	if (!obs_sceneitem_visible(item))
		return true;

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	matrix4_inv(&invTransform, &transform);
	vec3_transform(&transformedPos, &pos3, &invTransform);
	vec3_transform(&pos3_, &transformedPos, &transform);

	if (CloseFloat(pos3.x, pos3_.x) && CloseFloat(pos3.y, pos3_.y) && transformedPos.x >= 0.0f &&
	    transformedPos.x <= 1.0f && transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) {

		data->sceneItems.push_back(item);
		return true;
	}

	if (transform.t.x > x1 && transform.t.x < x2 && transform.t.y > y1 && transform.t.y < y2) {

		data->sceneItems.push_back(item);
		return true;
	}

	if (transform.t.x + transform.x.x > x1 && transform.t.x + transform.x.x < x2 &&
	    transform.t.y + transform.x.y > y1 && transform.t.y + transform.x.y < y2) {

		data->sceneItems.push_back(item);
		return true;
	}

	if (transform.t.x + transform.y.x > x1 && transform.t.x + transform.y.x < x2 &&
	    transform.t.y + transform.y.y > y1 && transform.t.y + transform.y.y < y2) {

		data->sceneItems.push_back(item);
		return true;
	}

	if (transform.t.x + transform.x.x + transform.y.x > x1 && transform.t.x + transform.x.x + transform.y.x < x2 &&
	    transform.t.y + transform.x.y + transform.y.y > y1 && transform.t.y + transform.x.y + transform.y.y < y2) {

		data->sceneItems.push_back(item);
		return true;
	}

	if (transform.t.x + 0.5 * (transform.x.x + transform.y.x) > x1 &&
	    transform.t.x + 0.5 * (transform.x.x + transform.y.x) < x2 &&
	    transform.t.y + 0.5 * (transform.x.y + transform.y.y) > y1 &&
	    transform.t.y + 0.5 * (transform.x.y + transform.y.y) < y2) {

		data->sceneItems.push_back(item);
		return true;
	}

	if (IntersectBox(transform, x1, x2, y1, y2)) {
		data->sceneItems.push_back(item);
		return true;
	}

	return true;
}

void OBSBasicPreview::BoxItems(const vec2 &startPos, const vec2 &pos)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSScene scene = main->GetCurrentScene();
	if (!scene)
		return;

	if (cursor().shape() != Qt::CrossCursor)
		setCursor(Qt::CrossCursor);

	SceneFindBoxData data(startPos, pos);
	obs_scene_enum_items(scene, FindItemsInBox, &data);

	std::lock_guard<std::mutex> lock(selectMutex);
	hoveredPreviewItems = data.sceneItems;
}

vec3 OBSBasicPreview::CalculateStretchPos(const vec3 &tl, const vec3 &br)
{
	uint32_t alignment = obs_sceneitem_get_alignment(stretchItem);
	vec3 pos;

	vec3_zero(&pos);

	if (alignment & OBS_ALIGN_LEFT)
		pos.x = tl.x;
	else if (alignment & OBS_ALIGN_RIGHT)
		pos.x = br.x;
	else
		pos.x = (br.x - tl.x) * 0.5f + tl.x;

	if (alignment & OBS_ALIGN_TOP)
		pos.y = tl.y;
	else if (alignment & OBS_ALIGN_BOTTOM)
		pos.y = br.y;
	else
		pos.y = (br.y - tl.y) * 0.5f + tl.y;

	return pos;
}

void OBSBasicPreview::ClampAspect(vec3 &tl, vec3 &br, vec2 &size, const vec2 &baseSize)
{
	float baseAspect = baseSize.x / baseSize.y;
	float aspect = size.x / size.y;
	uint32_t stretchFlags = (uint32_t)stretchHandle;

	if (stretchHandle == ItemHandle::TopLeft || stretchHandle == ItemHandle::TopRight ||
	    stretchHandle == ItemHandle::BottomLeft || stretchHandle == ItemHandle::BottomRight) {
		if (aspect < baseAspect) {
			if ((size.y >= 0.0f && size.x >= 0.0f) || (size.y <= 0.0f && size.x <= 0.0f))
				size.x = size.y * baseAspect;
			else
				size.x = size.y * baseAspect * -1.0f;
		} else {
			if ((size.y >= 0.0f && size.x >= 0.0f) || (size.y <= 0.0f && size.x <= 0.0f))
				size.y = size.x / baseAspect;
			else
				size.y = size.x / baseAspect * -1.0f;
		}

	} else if (stretchHandle == ItemHandle::TopCenter || stretchHandle == ItemHandle::BottomCenter) {
		if ((size.y >= 0.0f && size.x >= 0.0f) || (size.y <= 0.0f && size.x <= 0.0f))
			size.x = size.y * baseAspect;
		else
			size.x = size.y * baseAspect * -1.0f;

	} else if (stretchHandle == ItemHandle::CenterLeft || stretchHandle == ItemHandle::CenterRight) {
		if ((size.y >= 0.0f && size.x >= 0.0f) || (size.y <= 0.0f && size.x <= 0.0f))
			size.y = size.x / baseAspect;
		else
			size.y = size.x / baseAspect * -1.0f;
	}

	size.x = std::round(size.x);
	size.y = std::round(size.y);

	if (stretchFlags & ITEM_LEFT)
		tl.x = br.x - size.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x = tl.x + size.x;

	if (stretchFlags & ITEM_TOP)
		tl.y = br.y - size.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y = tl.y + size.y;
}

void OBSBasicPreview::SnapStretchingToScreen(vec3 &tl, vec3 &br)
{
	uint32_t stretchFlags = (uint32_t)stretchHandle;
	vec3 newTL = GetTransformedPos(tl.x, tl.y, itemToScreen);
	vec3 newTR = GetTransformedPos(br.x, tl.y, itemToScreen);
	vec3 newBL = GetTransformedPos(tl.x, br.y, itemToScreen);
	vec3 newBR = GetTransformedPos(br.x, br.y, itemToScreen);
	vec3 boundingTL;
	vec3 boundingBR;

	vec3_copy(&boundingTL, &newTL);
	vec3_min(&boundingTL, &boundingTL, &newTR);
	vec3_min(&boundingTL, &boundingTL, &newBL);
	vec3_min(&boundingTL, &boundingTL, &newBR);

	vec3_copy(&boundingBR, &newTL);
	vec3_max(&boundingBR, &boundingBR, &newTR);
	vec3_max(&boundingBR, &boundingBR, &newBL);
	vec3_max(&boundingBR, &boundingBR, &newBR);

	vec3 offset = GetSnapOffset(boundingTL, boundingBR);
	vec3_add(&offset, &offset, &newTL);
	vec3_transform(&offset, &offset, &screenToItem);
	vec3_sub(&offset, &offset, &tl);

	if (stretchFlags & ITEM_LEFT)
		tl.x += offset.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x += offset.x;

	if (stretchFlags & ITEM_TOP)
		tl.y += offset.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y += offset.y;
}

static float maxfunc(float x, float y)
{
	return x > y ? x : y;
}

static float minfunc(float x, float y)
{
	return x < y ? x : y;
}

void OBSBasicPreview::CropItem(const vec2 &pos)
{
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(stretchItem);
	uint32_t stretchFlags = (uint32_t)stretchHandle;
	uint32_t align = obs_sceneitem_get_alignment(stretchItem);
	vec3 tl, br, pos3;

	vec3_zero(&tl);
	vec3_set(&br, stretchItemSize.x, stretchItemSize.y, 0.0f);

	vec3_set(&pos3, pos.x, pos.y, 0.0f);
	vec3_transform(&pos3, &pos3, &screenToItem);

	obs_sceneitem_crop crop = startCrop;
	vec2 scale, rawscale;

	obs_sceneitem_get_scale(stretchItem, &rawscale);
	vec2_set(&scale, boundsType == OBS_BOUNDS_NONE ? rawscale.x : fabsf(rawscale.x),
		 boundsType == OBS_BOUNDS_NONE ? rawscale.y : fabsf(rawscale.y));

	vec2 max_tl;
	vec2 max_br;

	vec2_set(&max_tl, float(-crop.left) * scale.x, float(-crop.top) * scale.y);
	vec2_set(&max_br, stretchItemSize.x + crop.right * scale.x, stretchItemSize.y + crop.bottom * scale.y);

	typedef std::function<float(float, float)> minmax_func_t;

	minmax_func_t min_x = scale.x < 0.0f && boundsType == OBS_BOUNDS_NONE ? maxfunc : minfunc;
	minmax_func_t min_y = scale.y < 0.0f && boundsType == OBS_BOUNDS_NONE ? maxfunc : minfunc;
	minmax_func_t max_x = scale.x < 0.0f && boundsType == OBS_BOUNDS_NONE ? minfunc : maxfunc;
	minmax_func_t max_y = scale.y < 0.0f && boundsType == OBS_BOUNDS_NONE ? minfunc : maxfunc;

	pos3.x = min_x(pos3.x, max_br.x);
	pos3.x = max_x(pos3.x, max_tl.x);
	pos3.y = min_y(pos3.y, max_br.y);
	pos3.y = max_y(pos3.y, max_tl.y);

	if (stretchFlags & ITEM_LEFT) {
		float maxX = stretchItemSize.x - (2.0 * scale.x);
		pos3.x = tl.x = min_x(pos3.x, maxX);

	} else if (stretchFlags & ITEM_RIGHT) {
		float minX = (2.0 * scale.x);
		pos3.x = br.x = max_x(pos3.x, minX);
	}

	if (stretchFlags & ITEM_TOP) {
		float maxY = stretchItemSize.y - (2.0 * scale.y);
		pos3.y = tl.y = min_y(pos3.y, maxY);

	} else if (stretchFlags & ITEM_BOTTOM) {
		float minY = (2.0 * scale.y);
		pos3.y = br.y = max_y(pos3.y, minY);
	}

#define ALIGN_X (ITEM_LEFT | ITEM_RIGHT)
#define ALIGN_Y (ITEM_TOP | ITEM_BOTTOM)
	vec3 newPos;
	vec3_zero(&newPos);

	uint32_t align_x = (align & ALIGN_X);
	uint32_t align_y = (align & ALIGN_Y);
	if (align_x == (stretchFlags & ALIGN_X) && align_x != 0)
		newPos.x = pos3.x;
	else if (align & ITEM_RIGHT)
		newPos.x = stretchItemSize.x;
	else if (!(align & ITEM_LEFT))
		newPos.x = stretchItemSize.x * 0.5f;

	if (align_y == (stretchFlags & ALIGN_Y) && align_y != 0)
		newPos.y = pos3.y;
	else if (align & ITEM_BOTTOM)
		newPos.y = stretchItemSize.y;
	else if (!(align & ITEM_TOP))
		newPos.y = stretchItemSize.y * 0.5f;
#undef ALIGN_X
#undef ALIGN_Y

	crop = startCrop;

	if (stretchFlags & ITEM_LEFT)
		crop.left += int(std::round(tl.x / scale.x));
	else if (stretchFlags & ITEM_RIGHT)
		crop.right += int(std::round((stretchItemSize.x - br.x) / scale.x));

	if (stretchFlags & ITEM_TOP)
		crop.top += int(std::round(tl.y / scale.y));
	else if (stretchFlags & ITEM_BOTTOM)
		crop.bottom += int(std::round((stretchItemSize.y - br.y) / scale.y));

	vec3_transform(&newPos, &newPos, &itemToScreen);
	newPos.x = std::round(newPos.x);
	newPos.y = std::round(newPos.y);

#if 0
	vec3 curPos;
	vec3_zero(&curPos);
	obs_sceneitem_get_pos(stretchItem, (vec2*)&curPos);
	blog(LOG_DEBUG, "curPos {%d, %d} - newPos {%d, %d}",
			int(curPos.x), int(curPos.y),
			int(newPos.x), int(newPos.y));
	blog(LOG_DEBUG, "crop {%d, %d, %d, %d}",
			crop.left, crop.top,
			crop.right, crop.bottom);
#endif

	obs_sceneitem_defer_update_begin(stretchItem);
	obs_sceneitem_set_crop(stretchItem, &crop);
	if (boundsType == OBS_BOUNDS_NONE)
		obs_sceneitem_set_pos(stretchItem, (vec2 *)&newPos);
	obs_sceneitem_defer_update_end(stretchItem);
}

void OBSBasicPreview::StretchItem(const vec2 &pos)
{
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(stretchItem);
	uint32_t stretchFlags = (uint32_t)stretchHandle;
	bool shiftDown = (modifiers & Qt::ShiftModifier);
	vec3 tl, br, pos3;

	vec3_zero(&tl);
	vec3_set(&br, stretchItemSize.x, stretchItemSize.y, 0.0f);

	vec3_set(&pos3, pos.x, pos.y, 0.0f);
	vec3_transform(&pos3, &pos3, &screenToItem);

	if (stretchFlags & ITEM_LEFT)
		tl.x = pos3.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x = pos3.x;

	if (stretchFlags & ITEM_TOP)
		tl.y = pos3.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y = pos3.y;

	if (!(modifiers & Qt::ControlModifier))
		SnapStretchingToScreen(tl, br);

	obs_source_t *source = obs_sceneitem_get_source(stretchItem);

	uint32_t source_cx = obs_source_get_width(source);
	uint32_t source_cy = obs_source_get_height(source);

	/* if the source's internal size has been set to 0 for whatever reason
	 * while resizing, do not update transform, otherwise source will be
	 * stuck invisible until a complete transform reset */
	if (!source_cx || !source_cy)
		return;

	vec2 baseSize;
	vec2_set(&baseSize, float(source_cx), float(source_cy));

	vec2 size;
	vec2_set(&size, br.x - tl.x, br.y - tl.y);

	if (boundsType != OBS_BOUNDS_NONE) {
		if (shiftDown)
			ClampAspect(tl, br, size, baseSize);

		if (tl.x > br.x)
			std::swap(tl.x, br.x);
		if (tl.y > br.y)
			std::swap(tl.y, br.y);

		vec2_abs(&size, &size);

		obs_sceneitem_set_bounds(stretchItem, &size);
	} else {
		obs_sceneitem_crop crop;
		obs_sceneitem_get_crop(stretchItem, &crop);

		baseSize.x -= float(crop.left + crop.right);
		baseSize.y -= float(crop.top + crop.bottom);

		if (!shiftDown)
			ClampAspect(tl, br, size, baseSize);

		vec2_div(&size, &size, &baseSize);
		obs_sceneitem_set_scale(stretchItem, &size);
	}

	pos3 = CalculateStretchPos(tl, br);
	vec3_transform(&pos3, &pos3, &itemToScreen);

	vec2 newPos;
	vec2_set(&newPos, std::round(pos3.x), std::round(pos3.y));
	obs_sceneitem_set_pos(stretchItem, &newPos);
}

void OBSBasicPreview::RotateItem(const vec2 &pos)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	OBSScene scene = main->GetCurrentScene();
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	bool shiftDown = (modifiers & Qt::ShiftModifier);
	bool ctrlDown = (modifiers & Qt::ControlModifier);

	vec2 pos2;
	vec2_copy(&pos2, &pos);

	float angle = atan2(pos2.y - rotatePoint.y, pos2.x - rotatePoint.x) + RAD(90);

#define ROT_SNAP(rot, thresh)                      \
	if (abs(angle - RAD(rot)) < RAD(thresh)) { \
		angle = RAD(rot);                  \
	}

	if (shiftDown) {
		for (int i = 0; i <= 360 / 15; i++) {
			ROT_SNAP(i * 15 - 90, 7.5);
		}
	} else if (!ctrlDown) {
		ROT_SNAP(rotateAngle, 5)

		ROT_SNAP(-90, 5)
		ROT_SNAP(-45, 5)
		ROT_SNAP(0, 5)
		ROT_SNAP(45, 5)
		ROT_SNAP(90, 5)
		ROT_SNAP(135, 5)
		ROT_SNAP(180, 5)
		ROT_SNAP(225, 5)
		ROT_SNAP(270, 5)
		ROT_SNAP(315, 5)
	}
#undef ROT_SNAP

	vec2 pos3;
	vec2_copy(&pos3, &offsetPoint);
	RotatePos(&pos3, angle);
	pos3.x += rotatePoint.x;
	pos3.y += rotatePoint.y;

	obs_sceneitem_set_rot(stretchItem, DEG(angle));
	obs_sceneitem_set_pos(stretchItem, &pos3);
}

void OBSBasicPreview::mouseMoveEvent(QMouseEvent *event)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	changed = true;

	QPointF qtPos = event->position();

	float pixelRatio = main->GetDevicePixelRatio();

	if (scrollMode && event->buttons() == Qt::LeftButton) {
		scrollingOffset.x += pixelRatio * (qtPos.x() - scrollingFrom.x);
		scrollingOffset.y += pixelRatio * (qtPos.y() - scrollingFrom.y);
		scrollingFrom.x = qtPos.x();
		scrollingFrom.y = qtPos.y();
		emit DisplayResized();
		return;
	}

	if (locked)
		return;

	bool updateCursor = false;

	if (mouseDown) {
		vec2 pos = GetMouseEventPos(event);

		if (!mouseMoved && !mouseOverItems && stretchHandle == ItemHandle::None) {
			ProcessClick(startPos);
			mouseOverItems = SelectedAtPos(startPos);
		}

		pos.x = std::round(pos.x);
		pos.y = std::round(pos.y);

		if (stretchHandle != ItemHandle::None) {
			if (obs_sceneitem_locked(stretchItem))
				return;

			selectionBox = false;

			OBSScene scene = main->GetCurrentScene();
			obs_sceneitem_t *group = obs_sceneitem_get_group(scene, stretchItem);
			if (group) {
				vec3 group_pos;
				vec3_set(&group_pos, pos.x, pos.y, 0.0f);
				vec3_transform(&group_pos, &group_pos, &invGroupTransform);
				pos.x = group_pos.x;
				pos.y = group_pos.y;
			}

			if (stretchHandle == ItemHandle::Rot) {
				RotateItem(pos);
				setCursor(Qt::ClosedHandCursor);
			} else if (cropping)
				CropItem(pos);
			else
				StretchItem(pos);

		} else if (mouseOverItems) {
			if (cursor().shape() != Qt::SizeAllCursor)
				setCursor(Qt::SizeAllCursor);
			selectionBox = false;
			MoveItems(pos);
		} else {
			selectionBox = true;
			if (!mouseMoved)
				DoSelect(startPos);
			BoxItems(startPos, pos);
		}

		mouseMoved = true;
		mousePos = pos;
	} else {
		vec2 pos = GetMouseEventPos(event);
		OBSSceneItem item = GetItemAtPos(pos, true);

		std::lock_guard<std::mutex> lock(selectMutex);
		hoveredPreviewItems.clear();
		hoveredPreviewItems.push_back(item);

		if (!mouseMoved && hoveredPreviewItems.size() > 0) {
			mousePos = pos;
			OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
			float scale = main->GetDevicePixelRatio();
			float x = qtPos.x() - main->previewX / scale;
			float y = qtPos.y() - main->previewY / scale;
			vec2_set(&startPos, x, y);
			updateCursor = true;
		}
	}

	if (updateCursor) {
		GetStretchHandleData(startPos, true);
		uint32_t stretchFlags = (uint32_t)stretchHandle;
		UpdateCursor(stretchFlags);
	}
}

void OBSBasicPreview::leaveEvent(QEvent *)
{
	std::lock_guard<std::mutex> lock(selectMutex);
	if (!selectionBox)
		hoveredPreviewItems.clear();
}

static void DrawLine(float x1, float y1, float x2, float y2, float thickness, vec2 scale)
{
	float ySide = (y1 == y2) ? (y1 < 0.5f ? 1.0f : -1.0f) : 0.0f;
	float xSide = (x1 == x2) ? (x1 < 0.5f ? 1.0f : -1.0f) : 0.0f;

	gs_render_start(true);

	gs_vertex2f(x1 - (xSide * (thickness / scale.x) / 2), y1 + (ySide * (thickness / scale.y) / 2));
	gs_vertex2f(x1 + (xSide * (thickness / scale.x) / 2), y1 - (ySide * (thickness / scale.y) / 2));
	gs_vertex2f(x2 + (xSide * (thickness / scale.x) / 2), y2 + (ySide * (thickness / scale.y) / 2));
	gs_vertex2f(x2 - (xSide * (thickness / scale.x) / 2), y2 - (ySide * (thickness / scale.y) / 2));
	gs_vertex2f(x1 - (xSide * (thickness / scale.x) / 2), y1 + (ySide * (thickness / scale.y) / 2));

	gs_vertbuffer_t *line = gs_render_save();

	gs_load_vertexbuffer(line);
	gs_draw(GS_TRISTRIP, 0, 0);
	gs_vertexbuffer_destroy(line);
}

static void DrawSquareAtPos(float x, float y, float pixelRatio)
{
	struct vec3 pos;
	vec3_set(&pos, x, y, 0.0f);

	struct matrix4 matrix;
	gs_matrix_get(&matrix);
	vec3_transform(&pos, &pos, &matrix);

	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_translate(&pos);

	gs_matrix_translate3f(-HANDLE_RADIUS * pixelRatio, -HANDLE_RADIUS * pixelRatio, 0.0f);
	gs_matrix_scale3f(HANDLE_RADIUS * pixelRatio * 2, HANDLE_RADIUS * pixelRatio * 2, 1.0f);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_pop();
}

static void DrawRotationHandle(gs_vertbuffer_t *circle, float rot, float pixelRatio, bool invert)
{
	struct vec3 pos;
	vec3_set(&pos, 0.5f, invert ? 1.0f : 0.0f, 0.0f);

	struct matrix4 matrix;
	gs_matrix_get(&matrix);
	vec3_transform(&pos, &pos, &matrix);

	gs_render_start(true);

	gs_vertex2f(0.5f - 0.34f / HANDLE_RADIUS, 0.5f);
	gs_vertex2f(0.5f - 0.34f / HANDLE_RADIUS, -2.0f);
	gs_vertex2f(0.5f + 0.34f / HANDLE_RADIUS, -2.0f);
	gs_vertex2f(0.5f + 0.34f / HANDLE_RADIUS, 0.5f);
	gs_vertex2f(0.5f - 0.34f / HANDLE_RADIUS, 0.5f);

	gs_vertbuffer_t *line = gs_render_save();

	gs_load_vertexbuffer(line);

	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_translate(&pos);

	gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, RAD(rot));
	gs_matrix_translate3f(-HANDLE_RADIUS * 1.5 * pixelRatio, -HANDLE_RADIUS * 1.5 * pixelRatio, 0.0f);
	gs_matrix_scale3f(HANDLE_RADIUS * 3 * pixelRatio, HANDLE_RADIUS * 3 * pixelRatio, 1.0f);

	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_translate3f(0.0f, -HANDLE_RADIUS * 2 / 3, 0.0f);

	gs_load_vertexbuffer(circle);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_pop();
	gs_vertexbuffer_destroy(line);
}

static void DrawStripedLine(float x1, float y1, float x2, float y2, float thickness, vec2 scale)
{
	float ySide = (y1 == y2) ? (y1 < 0.5f ? 1.0f : -1.0f) : 0.0f;
	float xSide = (x1 == x2) ? (x1 < 0.5f ? 1.0f : -1.0f) : 0.0f;

	float dist = sqrt(pow((x1 - x2) * scale.x, 2) + pow((y1 - y2) * scale.y, 2));
	float offX = (x2 - x1) / dist;
	float offY = (y2 - y1) / dist;

	for (int i = 0, l = ceil(dist / 15); i < l; i++) {
		gs_render_start(true);

		float xx1 = x1 + i * 15 * offX;
		float yy1 = y1 + i * 15 * offY;

		float dx;
		float dy;

		if (x1 < x2) {
			dx = std::min(xx1 + 7.5f * offX, x2);
		} else {
			dx = std::max(xx1 + 7.5f * offX, x2);
		}

		if (y1 < y2) {
			dy = std::min(yy1 + 7.5f * offY, y2);
		} else {
			dy = std::max(yy1 + 7.5f * offY, y2);
		}

		gs_vertex2f(xx1, yy1);
		gs_vertex2f(xx1 + (xSide * (thickness / scale.x)), yy1 + (ySide * (thickness / scale.y)));
		gs_vertex2f(dx, dy);
		gs_vertex2f(dx + (xSide * (thickness / scale.x)), dy + (ySide * (thickness / scale.y)));

		gs_vertbuffer_t *line = gs_render_save();

		gs_load_vertexbuffer(line);
		gs_draw(GS_TRISTRIP, 0, 0);
		gs_vertexbuffer_destroy(line);
	}
}

static void DrawRect(float thickness, vec2 scale)
{
	gs_render_start(true);

	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f + (thickness / scale.x), 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(0.0f + (thickness / scale.x), 1.0f);
	gs_vertex2f(0.0f, 1.0f - (thickness / scale.y));
	gs_vertex2f(1.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f - (thickness / scale.y));
	gs_vertex2f(1.0f - (thickness / scale.x), 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f - (thickness / scale.x), 0.0f);
	gs_vertex2f(1.0f, 0.0f + (thickness / scale.y));
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 0.0f + (thickness / scale.y));

	gs_vertbuffer_t *rect = gs_render_save();

	gs_load_vertexbuffer(rect);
	gs_draw(GS_TRISTRIP, 0, 0);
	gs_vertexbuffer_destroy(rect);
}

static inline bool crop_enabled(const obs_sceneitem_crop *crop)
{
	return crop->left > 0 || crop->top > 0 || crop->right > 0 || crop->bottom > 0;
}

bool OBSBasicPreview::DrawSelectedOverflow(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_locked(item))
		return true;

	if (!SceneItemHasVideo(item))
		return true;

	OBSBasicPreview *prev = reinterpret_cast<OBSBasicPreview *>(param);

	if (!prev->GetOverflowSelectionHidden() && !obs_sceneitem_visible(item))
		return true;

	if (obs_sceneitem_is_group(item)) {
		matrix4 mat;
		obs_sceneitem_get_draw_transform(item, &mat);

		gs_matrix_push();
		gs_matrix_mul(&mat);
		obs_sceneitem_group_enum_items(item, DrawSelectedOverflow, param);
		gs_matrix_pop();
	}

	if (!prev->GetOverflowAlwaysVisible() && !obs_sceneitem_selected(item))
		return true;

	matrix4 boxTransform;
	matrix4 invBoxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);
	matrix4_inv(&invBoxTransform, &boxTransform);

	vec3 bounds[] = {
		{{{0.f, 0.f, 0.f}}},
		{{{1.f, 0.f, 0.f}}},
		{{{0.f, 1.f, 0.f}}},
		{{{1.f, 1.f, 0.f}}},
	};

	bool visible = std::all_of(std::begin(bounds), std::end(bounds), [&](const vec3 &b) {
		vec3 pos;
		vec3_transform(&pos, &b, &boxTransform);
		vec3_transform(&pos, &pos, &invBoxTransform);
		return CloseFloat(pos.x, b.x) && CloseFloat(pos.y, b.y);
	});

	if (!visible)
		return true;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawSelectedOverflow");

	obs_transform_info info;
	obs_sceneitem_get_info2(item, &info);

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_REPEAT);
	gs_eparam_t *image = gs_effect_get_param_by_name(solid, "image");
	gs_eparam_t *scale = gs_effect_get_param_by_name(solid, "scale");

	vec2 s;
	vec2_set(&s, boxTransform.x.x / 96, boxTransform.y.y / 96);

	gs_effect_set_vec2(scale, &s);
	gs_effect_set_texture(image, prev->overflow);

	gs_matrix_push();
	gs_matrix_mul(&boxTransform);

	obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(item, &crop);

	while (gs_effect_loop(solid, "Draw")) {
		gs_draw_sprite(prev->overflow, 0, 1, 1);
	}

	gs_matrix_pop();

	GS_DEBUG_MARKER_END();

	return true;
}

bool OBSBasicPreview::DrawSelectedItem(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_locked(item))
		return true;

	if (!SceneItemHasVideo(item))
		return true;

	OBSBasicPreview *prev = reinterpret_cast<OBSBasicPreview *>(param);

	if (obs_sceneitem_is_group(item)) {
		matrix4 mat;
		obs_transform_info groupInfo;
		obs_sceneitem_get_draw_transform(item, &mat);
		obs_sceneitem_get_info2(item, &groupInfo);

		prev->groupRot = groupInfo.rot;

		gs_matrix_push();
		gs_matrix_mul(&mat);
		obs_sceneitem_group_enum_items(item, DrawSelectedItem, prev);
		gs_matrix_pop();

		prev->groupRot = 0.0f;
	}

	OBSBasic *main = OBSBasic::Get();

	float pixelRatio = main->GetDevicePixelRatio();

	bool hovered = false;
	{
		std::lock_guard<std::mutex> lock(prev->selectMutex);
		for (const auto &hoveredItem : prev->hoveredPreviewItems) {
			if (hoveredItem == item) {
				hovered = true;
				break;
			}
		}
	}

	bool selected = obs_sceneitem_selected(item);

	if (!selected && !hovered)
		return true;

	matrix4 boxTransform;
	matrix4 invBoxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);
	matrix4_inv(&invBoxTransform, &boxTransform);

	vec3 bounds[] = {
		{{{0.f, 0.f, 0.f}}},
		{{{1.f, 0.f, 0.f}}},
		{{{0.f, 1.f, 0.f}}},
		{{{1.f, 1.f, 0.f}}},
	};

	main->GetCameraIcon();

	QColor selColor = main->GetSelectionColor();
	QColor cropColor = main->GetCropColor();
	QColor hoverColor = main->GetHoverColor();

	vec4 red;
	vec4 green;
	vec4 blue;

	vec4_set(&red, selColor.redF(), selColor.greenF(), selColor.blueF(), 1.0f);
	vec4_set(&green, cropColor.redF(), cropColor.greenF(), cropColor.blueF(), 1.0f);
	vec4_set(&blue, hoverColor.redF(), hoverColor.greenF(), hoverColor.blueF(), 1.0f);

	bool visible = std::all_of(std::begin(bounds), std::end(bounds), [&](const vec3 &b) {
		vec3 pos;
		vec3_transform(&pos, &b, &boxTransform);
		vec3_transform(&pos, &pos, &invBoxTransform);
		return CloseFloat(pos.x, b.x) && CloseFloat(pos.y, b.y);
	});

	if (!visible)
		return true;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawSelectedItem");

	matrix4 curTransform;
	vec2 boxScale;
	gs_matrix_get(&curTransform);
	obs_sceneitem_get_box_scale(item, &boxScale);
	boxScale.x *= curTransform.x.x;
	boxScale.y *= curTransform.y.y;

	obs_transform_info info;
	obs_sceneitem_get_info2(item, &info);

	gs_matrix_push();
	gs_matrix_mul(&boxTransform);

	obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(item, &crop);

	gs_effect_t *eff = gs_get_effect();
	gs_eparam_t *colParam = gs_effect_get_param_by_name(eff, "color");

	gs_effect_set_vec4(colParam, &red);

	if (info.bounds_type == OBS_BOUNDS_NONE && crop_enabled(&crop)) {
#define DRAW_SIDE(side, x1, y1, x2, y2)                                                   \
	if (hovered && !selected) {                                                       \
		gs_effect_set_vec4(colParam, &blue);                                      \
		DrawLine(x1, y1, x2, y2, HANDLE_RADIUS *pixelRatio / 2, boxScale);        \
	} else if (crop.side > 0) {                                                       \
		gs_effect_set_vec4(colParam, &green);                                     \
		DrawStripedLine(x1, y1, x2, y2, HANDLE_RADIUS *pixelRatio / 2, boxScale); \
	} else {                                                                          \
		DrawLine(x1, y1, x2, y2, HANDLE_RADIUS *pixelRatio / 2, boxScale);        \
	}                                                                                 \
	gs_effect_set_vec4(colParam, &red);

		DRAW_SIDE(left, 0.0f, 0.0f, 0.0f, 1.0f);
		DRAW_SIDE(top, 0.0f, 0.0f, 1.0f, 0.0f);
		DRAW_SIDE(right, 1.0f, 0.0f, 1.0f, 1.0f);
		DRAW_SIDE(bottom, 0.0f, 1.0f, 1.0f, 1.0f);
#undef DRAW_SIDE
	} else {
		if (!selected) {
			gs_effect_set_vec4(colParam, &blue);
			DrawRect(HANDLE_RADIUS * pixelRatio / 2, boxScale);
		} else {
			DrawRect(HANDLE_RADIUS * pixelRatio / 2, boxScale);
		}
	}

	gs_load_vertexbuffer(main->box);
	gs_effect_set_vec4(colParam, &red);

	if (selected) {
		DrawSquareAtPos(0.0f, 0.0f, pixelRatio);
		DrawSquareAtPos(0.0f, 1.0f, pixelRatio);
		DrawSquareAtPos(1.0f, 0.0f, pixelRatio);
		DrawSquareAtPos(1.0f, 1.0f, pixelRatio);
		DrawSquareAtPos(0.5f, 0.0f, pixelRatio);
		DrawSquareAtPos(0.0f, 0.5f, pixelRatio);
		DrawSquareAtPos(0.5f, 1.0f, pixelRatio);
		DrawSquareAtPos(1.0f, 0.5f, pixelRatio);

		if (!prev->circleFill) {
			gs_render_start(true);

			float angle = 180;
			for (int i = 0, l = 40; i < l; i++) {
				gs_vertex2f(sin(RAD(angle)) / 2 + 0.5f, cos(RAD(angle)) / 2 + 0.5f);
				angle += 360 / l;
				gs_vertex2f(sin(RAD(angle)) / 2 + 0.5f, cos(RAD(angle)) / 2 + 0.5f);
				gs_vertex2f(0.5f, 1.0f);
			}

			prev->circleFill = gs_render_save();
		}

		bool invert = info.scale.y < 0.0f && info.bounds_type == OBS_BOUNDS_NONE;
		DrawRotationHandle(prev->circleFill, info.rot + prev->groupRot, pixelRatio, invert);
	}

	gs_matrix_pop();

	GS_DEBUG_MARKER_END();

	return true;
}

bool OBSBasicPreview::DrawSelectionBox(float x1, float y1, float x2, float y2, gs_vertbuffer_t *rectFill)
{
	OBSBasic *main = OBSBasic::Get();

	float pixelRatio = main->GetDevicePixelRatio();

	x1 = std::round(x1);
	x2 = std::round(x2);
	y1 = std::round(y1);
	y2 = std::round(y2);

	gs_effect_t *eff = gs_get_effect();
	gs_eparam_t *colParam = gs_effect_get_param_by_name(eff, "color");

	vec4 fillColor;
	vec4_set(&fillColor, 0.7f, 0.7f, 0.7f, 0.5f);

	vec4 borderColor;
	vec4_set(&borderColor, 1.0f, 1.0f, 1.0f, 1.0f);

	vec2 scale;
	vec2_set(&scale, std::abs(x2 - x1), std::abs(y2 - y1));

	gs_matrix_push();
	gs_matrix_identity();

	gs_matrix_translate3f(x1, y1, 0.0f);
	gs_matrix_scale3f(x2 - x1, y2 - y1, 1.0f);

	gs_effect_set_vec4(colParam, &fillColor);
	gs_load_vertexbuffer(rectFill);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_effect_set_vec4(colParam, &borderColor);
	DrawRect(HANDLE_RADIUS * pixelRatio / 2, scale);

	gs_matrix_pop();

	return true;
}

void OBSBasicPreview::DrawOverflow()
{
	if (locked)
		return;

	if (overflowHidden)
		return;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawOverflow");

	if (!overflow) {
		std::string path;
		GetDataFilePath("images/overflow.png", path);
		overflow = gs_texture_create_from_file(path.c_str());
	}

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	OBSScene scene = main->GetCurrentScene();

	if (scene) {
		gs_matrix_push();
		gs_matrix_scale3f(main->previewScale, main->previewScale, 1.0f);
		obs_scene_enum_items(scene, DrawSelectedOverflow, this);
		gs_matrix_pop();
	}

	gs_load_vertexbuffer(nullptr);

	GS_DEBUG_MARKER_END();
}

void OBSBasicPreview::DrawSceneEditing()
{
	if (locked)
		return;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawSceneEditing");

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	OBSScene scene = main->GetCurrentScene();

	if (scene) {
		gs_matrix_push();
		gs_matrix_scale3f(main->previewScale, main->previewScale, 1.0f);
		obs_scene_enum_items(scene, DrawSelectedItem, this);
		gs_matrix_pop();
	}

	if (selectionBox) {
		if (!rectFill) {
			gs_render_start(true);

			gs_vertex2f(0.0f, 0.0f);
			gs_vertex2f(1.0f, 0.0f);
			gs_vertex2f(0.0f, 1.0f);
			gs_vertex2f(1.0f, 1.0f);

			rectFill = gs_render_save();
		}

		DrawSelectionBox(startPos.x * main->previewScale, startPos.y * main->previewScale,
				 mousePos.x * main->previewScale, mousePos.y * main->previewScale, rectFill);
	}

	gs_load_vertexbuffer(nullptr);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);

	GS_DEBUG_MARKER_END();
}

void OBSBasicPreview::ResetScrollingOffset()
{
	vec2_zero(&scrollingOffset);
}

void OBSBasicPreview::SetScalingLevel(int32_t newScalingLevelVal)
{
	newScalingLevelVal = std::clamp(newScalingLevelVal, -MAX_SCALING_LEVEL, MAX_SCALING_LEVEL);
	float newScalingAmountVal = pow(ZOOM_SENSITIVITY, float(newScalingLevelVal));
	scalingLevel = newScalingLevelVal;
	SetScalingAmount(newScalingAmountVal);
}

void OBSBasicPreview::SetScalingAmount(float newScalingAmountVal)
{
	scrollingOffset.x *= newScalingAmountVal / scalingAmount;
	scrollingOffset.y *= newScalingAmountVal / scalingAmount;

	if (scalingAmount == newScalingAmountVal)
		return;

	scalingAmount = newScalingAmountVal;
	emit scalingChanged(scalingAmount);
}

void OBSBasicPreview::SetScalingLevelAndAmount(int32_t newScalingLevelVal, float newScalingAmountVal)
{
	newScalingLevelVal = std::clamp(newScalingLevelVal, -MAX_SCALING_LEVEL, MAX_SCALING_LEVEL);
	scalingLevel = newScalingLevelVal;
	SetScalingAmount(newScalingAmountVal);
}

OBSBasicPreview *OBSBasicPreview::Get()
{
	return OBSBasic::Get()->ui->preview;
}

static obs_source_t *CreateLabel(float pixelRatio, int i)
{
	OBSDataAutoRelease settings = obs_data_create();
	OBSDataAutoRelease font = obs_data_create();

#if defined(_WIN32)
	obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
	obs_data_set_string(font, "face", "Helvetica");
#else
	obs_data_set_string(font, "face", "Monospace");
#endif
	obs_data_set_int(font, "flags", 1); // Bold text
	obs_data_set_int(font, "size", 16 * pixelRatio);

	obs_data_set_obj(settings, "font", font);
	obs_data_set_bool(settings, "outline", true);

#ifdef _WIN32
	obs_data_set_int(settings, "outline_color", 0x000000);
	obs_data_set_int(settings, "outline_size", 3);
	const char *text_source_id = "text_gdiplus";
#else
	const char *text_source_id = "text_ft2_source";
#endif

	DStr name;
	dstr_printf(name, "Preview spacing label %d", i);
	return obs_source_create_private(text_source_id, name, settings);
}

static void SetLabelText(int sourceIndex, int px)
{
	OBSBasicPreview *prev = OBSBasicPreview::Get();

	if (px == prev->spacerPx[sourceIndex])
		return;

	std::string text = std::to_string(px) + " px";

	obs_source_t *source = prev->spacerLabel[sourceIndex];

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "text", text.c_str());
	obs_source_update(source, settings);

	prev->spacerPx[sourceIndex] = px;
}

static void DrawLabel(OBSSource source, vec3 &pos, vec3 &viewport)
{
	if (!source)
		return;

	vec3_mul(&pos, &pos, &viewport);

	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_translate(&pos);
	obs_source_video_render(source);
	gs_matrix_pop();
}

static void DrawSpacingLine(vec3 &start, vec3 &end, vec3 &viewport, float pixelRatio)
{
	OBSBasic *main = OBSBasic::Get();

	matrix4 transform;
	matrix4_identity(&transform);
	transform.x.x = viewport.x;
	transform.y.y = viewport.y;

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	QColor selColor = main->GetSelectionColor();
	vec4 color;
	vec4_set(&color, selColor.redF(), selColor.greenF(), selColor.blueF(), 1.0f);

	gs_effect_set_vec4(gs_effect_get_param_by_name(solid, "color"), &color);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	gs_matrix_push();
	gs_matrix_mul(&transform);

	vec2 scale;
	vec2_set(&scale, viewport.x, viewport.y);

	DrawLine(start.x, start.y, end.x, end.y, pixelRatio * (HANDLE_RADIUS / 2), scale);

	gs_matrix_pop();

	gs_load_vertexbuffer(nullptr);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

static void RenderSpacingHelper(int sourceIndex, vec3 &start, vec3 &end, vec3 &viewport, float pixelRatio)
{
	bool horizontal = (sourceIndex == 2 || sourceIndex == 3);

	// If outside of preview, don't render
	if (!((horizontal && (end.x >= start.x)) || (!horizontal && (end.y >= start.y))))
		return;

	float length = vec3_dist(&start, &end);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	float px;

	if (horizontal) {
		px = length * ovi.base_width;
	} else {
		px = length * ovi.base_height;
	}

	if (px <= 0.0f)
		return;

	OBSBasicPreview *prev = OBSBasicPreview::Get();
	obs_source_t *source = prev->spacerLabel[sourceIndex];
	vec3 labelSize, labelPos;
	vec3_set(&labelSize, obs_source_get_width(source), obs_source_get_height(source), 1.0f);

	vec3_div(&labelSize, &labelSize, &viewport);

	vec3 labelMargin;
	vec3_set(&labelMargin, SPACER_LABEL_MARGIN * pixelRatio, SPACER_LABEL_MARGIN * pixelRatio, 1.0f);
	vec3_div(&labelMargin, &labelMargin, &viewport);

	vec3_set(&labelPos, end.x, end.y, end.z);
	if (horizontal) {
		labelPos.x -= (end.x - start.x) / 2;
		labelPos.x -= labelSize.x / 2;
		labelPos.y -= labelMargin.y + (labelSize.y / 2) + (HANDLE_RADIUS / viewport.y);
	} else {
		labelPos.y -= (end.y - start.y) / 2;
		labelPos.y -= labelSize.y / 2;
		labelPos.x += labelMargin.x;
	}

	DrawSpacingLine(start, end, viewport, pixelRatio);
	SetLabelText(sourceIndex, (int)px);
	DrawLabel(source, labelPos, viewport);
}

void OBSBasicPreview::DrawSpacingHelpers()
{
	if (locked)
		return;

	OBSBasic *main = OBSBasic::Get();

	vec2 s;
	SceneFindBoxData data(s, s);

	OBSScene scene = main->GetCurrentScene();
	obs_scene_enum_items(scene, FindSelected, &data);

	if (data.sceneItems.size() != 1)
		return;

	OBSSceneItem item = data.sceneItems[0];
	if (!item)
		return;

	if (obs_sceneitem_locked(item))
		return;

	vec2 itemSize = GetItemSize(item);
	if (itemSize.x == 0.0f || itemSize.y == 0.0f)
		return;

	obs_sceneitem_t *parentGroup = obs_sceneitem_get_group(scene, item);
	if (parentGroup && obs_sceneitem_locked(parentGroup))
		return;

	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	obs_transform_info oti;
	obs_sceneitem_get_info2(item, &oti);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	vec3 size;
	vec3_set(&size, ovi.base_width, ovi.base_height, 1.0f);

	// Init box transform side locations
	vec3 left, right, top, bottom;

	vec3_set(&left, 0.0f, 0.5f, 1.0f);
	vec3_set(&right, 1.0f, 0.5f, 1.0f);
	vec3_set(&top, 0.5f, 0.0f, 1.0f);
	vec3_set(&bottom, 0.5f, 1.0f, 1.0f);

	// Decide which side to use with box transform, based on rotation
	// Seems hacky, probably a better way to do it
	float rot = oti.rot;

	if (parentGroup) {
		obs_transform_info groupOti;
		obs_sceneitem_get_info2(parentGroup, &groupOti);

		//Correct the scene item rotation angle
		rot = oti.rot + groupOti.rot;

		// Correct the scene item box transform
		// Based on scale, rotation angle, position of parent's group
		matrix4_scale3f(&boxTransform, &boxTransform, groupOti.scale.x, groupOti.scale.y, 1.0f);
		matrix4_rotate_aa4f(&boxTransform, &boxTransform, 0.0f, 0.0f, 1.0f, RAD(groupOti.rot));
		matrix4_translate3f(&boxTransform, &boxTransform, groupOti.pos.x, groupOti.pos.y, 0.0f);
	}

	// Switch top/bottom or right/left if scale is negative
	if (oti.scale.x < 0.0f && oti.bounds_type == OBS_BOUNDS_NONE) {
		vec3 l = left;
		vec3 r = right;

		vec3_copy(&left, &r);
		vec3_copy(&right, &l);
	}

	if (oti.scale.y < 0.0f && oti.bounds_type == OBS_BOUNDS_NONE) {
		vec3 t = top;
		vec3 b = bottom;

		vec3_copy(&top, &b);
		vec3_copy(&bottom, &t);
	}

	if (rot >= HELPER_ROT_BREAKPOINT) {
		for (float i = HELPER_ROT_BREAKPOINT; i <= 360.0f; i += 90.0f) {
			if (rot < i)
				break;

			vec3 l = left;
			vec3 r = right;
			vec3 t = top;
			vec3 b = bottom;

			vec3_copy(&top, &l);
			vec3_copy(&right, &t);
			vec3_copy(&bottom, &r);
			vec3_copy(&left, &b);
		}
	} else if (rot <= -HELPER_ROT_BREAKPOINT) {
		for (float i = -HELPER_ROT_BREAKPOINT; i >= -360.0f; i -= 90.0f) {
			if (rot > i)
				break;

			vec3 l = left;
			vec3 r = right;
			vec3 t = top;
			vec3 b = bottom;

			vec3_copy(&top, &r);
			vec3_copy(&right, &b);
			vec3_copy(&bottom, &l);
			vec3_copy(&left, &t);
		}
	}

	// Get sides of box transform
	left = GetTransformedPos(left.x, left.y, boxTransform);
	right = GetTransformedPos(right.x, right.y, boxTransform);
	top = GetTransformedPos(top.x, top.y, boxTransform);
	bottom = GetTransformedPos(bottom.x, bottom.y, boxTransform);

	bottom.y = size.y - bottom.y;
	right.x = size.x - right.x;

	// Init viewport
	vec3 viewport;
	vec3_set(&viewport, main->previewCX, main->previewCY, 1.0f);

	vec3_div(&left, &left, &viewport);
	vec3_div(&right, &right, &viewport);
	vec3_div(&top, &top, &viewport);
	vec3_div(&bottom, &bottom, &viewport);

	vec3_mulf(&left, &left, main->previewScale);
	vec3_mulf(&right, &right, main->previewScale);
	vec3_mulf(&top, &top, main->previewScale);
	vec3_mulf(&bottom, &bottom, main->previewScale);

	// Draw spacer lines and labels
	vec3 start, end;

	float pixelRatio = main->GetDevicePixelRatio();
	for (int i = 0; i < 4; i++) {
		if (!spacerLabel[i])
			spacerLabel[i] = CreateLabel(pixelRatio, i);
	}

	vec3_set(&start, top.x, 0.0f, 1.0f);
	vec3_set(&end, top.x, top.y, 1.0f);
	RenderSpacingHelper(0, start, end, viewport, pixelRatio);

	vec3_set(&start, bottom.x, 1.0f - bottom.y, 1.0f);
	vec3_set(&end, bottom.x, 1.0f, 1.0f);
	RenderSpacingHelper(1, start, end, viewport, pixelRatio);

	vec3_set(&start, 0.0f, left.y, 1.0f);
	vec3_set(&end, left.x, left.y, 1.0f);
	RenderSpacingHelper(2, start, end, viewport, pixelRatio);

	vec3_set(&start, 1.0f - right.x, right.y, 1.0f);
	vec3_set(&end, 1.0f, right.y, 1.0f);
	RenderSpacingHelper(3, start, end, viewport, pixelRatio);
}

void OBSBasicPreview::ClampScrollingOffsets()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	QSize targetSize = GetPixelSize(this);

	vec3 target, offset;
	vec3_set(&target, (float)targetSize.width(), (float)targetSize.height(), 1.0f);

	vec3_set(&offset, (float)ovi.base_width, (float)ovi.base_height, 1.0f);
	vec3_mulf(&offset, &offset, scalingAmount);

	vec3_sub(&offset, &offset, &target);

	vec3_mulf(&offset, &offset, 0.5f);
	vec3_maxf(&offset, &offset, 0.0f);

	vec3_divf(&target, &target, 2.0f);
	vec3_add(&offset, &offset, &target);

	scrollingOffset.x = std::clamp(scrollingOffset.x, -offset.x, offset.x);
	scrollingOffset.y = std::clamp(scrollingOffset.y, -offset.y, offset.y);

	UpdateXScrollBar(offset.x);
	UpdateYScrollBar(offset.y);
}

void OBSBasicPreview::XScrollBarMoved(int value)
{
	updatingXScrollBar = true;
	scrollingOffset.x = float(-value);

	emit DisplayResized();
	updatingXScrollBar = false;
}

void OBSBasicPreview::YScrollBarMoved(int value)
{
	updatingYScrollBar = true;
	scrollingOffset.y = float(-value);

	emit DisplayResized();
	updatingYScrollBar = false;
}

void OBSBasicPreview::UpdateXScrollBar(float cx)
{
	if (updatingXScrollBar)
		return;

	OBSBasic *main = OBSBasic::Get();

	if (!main->ui->previewXScrollBar->isVisible())
		return;

	main->ui->previewXScrollBar->setRange(int(-cx), int(cx));

	QSize targetSize = GetPixelSize(this);
	main->ui->previewXScrollBar->setPageStep(targetSize.width() / std::min(scalingAmount, 1.0f));

	QSignalBlocker sig(main->ui->previewXScrollBar);
	main->ui->previewXScrollBar->setValue(int(-scrollingOffset.x));
}

void OBSBasicPreview::UpdateYScrollBar(float cy)
{
	if (updatingYScrollBar)
		return;

	OBSBasic *main = OBSBasic::Get();

	if (!main->ui->previewYScrollBar->isVisible())
		return;

	main->ui->previewYScrollBar->setRange(int(-cy), int(cy));

	QSize targetSize = GetPixelSize(this);
	main->ui->previewYScrollBar->setPageStep(targetSize.height() / std::min(scalingAmount, 1.0f));

	QSignalBlocker sig(main->ui->previewYScrollBar);
	main->ui->previewYScrollBar->setValue(int(-scrollingOffset.y));
}
