#include "OBSClipboardSerializer.hpp"

OBSData OBSClipboardSerializer::SerializeSceneItem(OBSSceneItem item)
{
	if (!item) {
		return {};
	}

	return {};
}

OBSData OBSClipboardSerializer::SerializeFilters(OBSSource source)
{
	if (!source) {
		return {};
	}
	return {};
}

OBSData OBSClipboardSerializer::SerializeTransform(OBSSceneItem item)
{
	if (!item) {
		return {};
	}

	obs_transform_info transform;
	obs_sceneitem_crop crop;

	obs_sceneitem_get_info2(item, &transform);
	obs_sceneitem_get_crop(item, &crop);

	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_double(data, "pos_x", transform.pos.x);
	obs_data_set_double(data, "pos_y", transform.pos.y);
	obs_data_set_double(data, "rot", transform.rot);
	obs_data_set_double(data, "scale_x", transform.scale.x);
	obs_data_set_double(data, "scale_y", transform.scale.y);
	obs_data_set_int(data, "alignment", transform.alignment);
	obs_data_set_int(data, "bounds_type", transform.bounds_type);
	obs_data_set_int(data, "bounds_alignment", transform.bounds_alignment);
	obs_data_set_double(data, "bounds_x", transform.bounds.x);
	obs_data_set_double(data, "bounds_y", transform.bounds.y);

	obs_data_set_int(data, "crop_left", crop.left);
	obs_data_set_int(data, "crop_top", crop.top);
	obs_data_set_int(data, "crop_right", crop.right);
	obs_data_set_int(data, "crop_bottom", crop.bottom);

	return OBSData(data);
}

OBSData OBSClipboardSerializer::SerializeTransition(OBSSceneItem item, bool show)
{
	if (!item) {
		return {};
	}
	return {};
}

bool OBSClipboardSerializer::DeserializeSceneItem(OBSData data)
{
	return false;
}

bool OBSClipboardSerializer::DeserializeFilters(OBSData data)
{
	return false;
}

bool OBSClipboardSerializer::DeserializeTransform(OBSData data, obs_transform_info &transform, obs_sceneitem_crop &crop)
{
	if (!data) {
		return false;
	}
	transform.pos.x = obs_data_get_double(data, "pos_x");
	transform.pos.y = obs_data_get_double(data, "pos_y");
	transform.rot = obs_data_get_double(data, "rot");
	transform.scale.x = obs_data_get_double(data, "scale_x");
	transform.scale.y = obs_data_get_double(data, "scale_y");
	transform.alignment = obs_data_get_int(data, "alignment");
	transform.bounds_type = static_cast<obs_bounds_type>(obs_data_get_int(data, "bounds_type"));
	transform.bounds_alignment = obs_data_get_int(data, "bounds_alignment");
	transform.bounds.x = obs_data_get_double(data, "bounds_x");
	transform.bounds.y = obs_data_get_double(data, "bounds_y");

	crop.left = obs_data_get_int(data, "crop_left");
	crop.top = obs_data_get_int(data, "crop_top");
	crop.right = obs_data_get_int(data, "crop_right");
	crop.bottom = obs_data_get_int(data, "crop_bottom");

	return true;
}

bool OBSClipboardSerializer::DeserializeTransition(OBSData data)
{
	return false;
}
