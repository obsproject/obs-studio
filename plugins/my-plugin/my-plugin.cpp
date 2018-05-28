#include <obs-module.h>
#include <algorithm>
#include <string>
#include <memory>

using namespace std;



bool obs_module_load(void)
{
	obs_source_info si = {};
	si.id = "my-plugin";
	si.type = OBS_SOURCE_TYPE_INPUT;
	si.output_flags = OBS_SOURCE_VIDEO;
	si.get_properties = get_properties;

	si.get_name = [] (void*)
	{
		return obs_module_text("TextGDIPlus");
	};
	si.create = [] (obs_data_t *settings, obs_source_t *source)
	{
		return ;
	};
	si.destroy = [] (void *data)
	{
		return ;
	};
	si.get_width = [] (void *data)
	{
		return ;
	};
	si.get_height = [] (void *data)
	{
		return ;
	};
	si.get_defaults = [] (obs_data_t *settings)
	{
		obs_data_t *font_obj = obs_data_create();
		obs_data_set_default_string(font_obj, "face", "Arial");
		obs_data_set_default_int(font_obj, "size", 36);

		obs_data_release(font_obj);
	};
	si.update = [] (void *data, obs_data_t *settings)
	{
		return;
	};
	si.video_tick = [] (void *data, float seconds)
	{
		return;
	};
	si.video_render = [] (void *data, gs_effect_t *effect)
	{
		return;
	};

	obs_register_source(&si);

	return true;
}

void obs_module_unload(void)
{

}
