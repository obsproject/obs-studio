#include "OBSClipboardSerializer.hpp"

OBSData OBSClipboardSerializer::SerializeSceneItem(OBSSceneItem item)
{
	if (!item) {
		return {};
	}

	OBSSource source = obs_sceneitem_get_source(item);
	if (!source) {
		return {};
	}
	OBSDataArrayAutoRelease items = obs_data_array_create();
	obs_sceneitem_save(item, items);

	OBSDataAutoRelease sourceData = obs_save_source(source);
	if (!sourceData) {
		return {};
	}
	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_array(data, "items", items);
	obs_data_set_obj(data, "source", sourceData);
	obs_data_set_int(data, "output_flags", obs_source_get_output_flags(source));

	return OBSData(data);
}

OBSData OBSClipboardSerializer::SerializeFilters(OBSSource source)
{
	if (!source) {
		return {};
	}
	OBSDataArrayAutoRelease filters = obs_source_backup_filters(source);
	if (!filters) {
		return {};
	}
	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_array(data, "filters", filters);

	return OBSData(data);
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
	OBSDataAutoRelease data = obs_sceneitem_transition_save(item, show);
	return OBSData(data);
}

bool OBSClipboardSerializer::DeserializeSceneItem(const OBSDataAutoRelease &data, OBSDataArrayAutoRelease &items,
						  OBSDataAutoRelease &source, uint32_t &outputFlags)
{
	if (!data) {
		return false;
	}
	items = obs_data_get_array(data, "items");
	source = obs_data_get_obj(data, "source");
	outputFlags = static_cast<uint32_t>(obs_data_get_int(data, "output_flags"));
	return !!items && obs_data_array_count(items) && !!source;
}

bool OBSClipboardSerializer::DeserializeFilters(const OBSData &data, OBSDataArrayAutoRelease &filters)
{
	if (!data) {
		return false;
	}
	filters = obs_data_get_array(data, "filters");
	return !!filters;
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

// not needed as obs_sceneitem_transition_load does the job for us
// bool OBSClipboardSerializer::DeserializeTransition(const OBSData &data)
// {
// 	return false;
// }
