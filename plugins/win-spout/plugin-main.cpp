#include "SpoutPlugin.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-spout", "en-US")

static const char *spout_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Spout");
}

static void *spout_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	SpoutPlugin* s = new SpoutPlugin();
	s->source = source;
	return s;
}

static void spout_destroy(void *data)
{
	SpoutPlugin* s = (SpoutPlugin*)data;
	delete s;
}

static obs_properties_t *spout_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *  list = obs_properties_add_list(
		props, "source_name", "Source", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(list, "{Please select a source}", nullptr);

	SpoutPlugin* s = (SpoutPlugin*)data;
	std::vector<std::string> names = s->GetSenderNames();

	for (auto &sourceName : names) {
		obs_property_list_add_string(list, sourceName.c_str(), sourceName.c_str());
	}

	UNUSED_PARAMETER(data);

	return props;
}

static uint32_t spout_get_width(void *data)
{
	SpoutPlugin* s = (SpoutPlugin*)data;
	return s->GetWidth();
}

static uint32_t spout_get_height(void *data)
{
	SpoutPlugin* s = (SpoutPlugin*)data;
	return s->GetHeight();
}

static void spout_update(void *data, obs_data_t *settings)
{
	const char *sourceName = obs_data_get_string(settings, "source_name");

	SpoutPlugin* s = (SpoutPlugin*)data;
	s->SetActiveSender(sourceName);
}

static void spout_tick(void *data, float seconds) {
	SpoutPlugin* s = (SpoutPlugin*)data;

	s->Tick();
}

bool obs_module_load(void)
{
	struct obs_source_info spout_info = {};
	spout_info.id = "spout-input";
	spout_info.type = OBS_SOURCE_TYPE_INPUT;
	spout_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
	OBS_SOURCE_DO_NOT_DUPLICATE;
	spout_info.get_name = spout_get_name;
	spout_info.create = spout_create;
	spout_info.destroy = spout_destroy;
	//spout_info.video_render = spout_video_render;
	spout_info.get_properties = spout_properties;
	spout_info.get_width = spout_get_width;
	spout_info.get_height = spout_get_height;
	spout_info.update = spout_update;
	spout_info.video_tick = spout_tick;

	obs_register_source(&spout_info);
	return true;
}