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
	obs_data_set_vec2(data, "pos", &transform.pos);
	obs_data_set_double(data, "rot", transform.rot);
	obs_data_set_vec2(data, "scale", &transform.scale);
	obs_data_set_int(data, "alignment", transform.alignment);
	obs_data_set_int(data, "bounds_type", transform.bounds_type);
	obs_data_set_int(data, "bounds_alignment", transform.bounds_alignment);
	obs_data_set_vec2(data, "bounds", &transform.bounds);
	obs_data_set_bool(data, "crop_to_bounds", transform.crop_to_bounds);

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

bool OBSClipboardSerializer::DeserializeTransform(const OBSData &data, obs_transform_info &transform,
						  obs_sceneitem_crop &crop)
{
	if (!data) {
		return false;
	}
	obs_data_get_vec2(data, "pos", &transform.pos);
	transform.rot = obs_data_get_double(data, "rot");
	obs_data_get_vec2(data, "scale", &transform.scale);
	transform.alignment = obs_data_get_int(data, "alignment");
	transform.bounds_type = static_cast<obs_bounds_type>(obs_data_get_int(data, "bounds_type"));
	transform.bounds_alignment = obs_data_get_int(data, "bounds_alignment");
	obs_data_get_vec2(data, "bounds", &transform.bounds);
	transform.crop_to_bounds = obs_data_get_bool(data, "crop_to_bounds");

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
