#include <obs-module.h>

#include "xcompcap-main.h"

static void* xcompcap_create(obs_data_t settings, obs_source_t source)
{
	return new XCompcapMain(settings, source);
}

static void xcompcap_destroy(void *data)
{
	XCompcapMain* cc = (XCompcapMain*)data;
	delete cc;
}

static void xcompcap_video_tick(void* data, float seconds)
{
	XCompcapMain* cc = (XCompcapMain*)data;
	cc->tick(seconds);
}

static void xcompcap_video_render(void* data, effect_t effect)
{
	XCompcapMain* cc = (XCompcapMain*)data;
	cc->render(effect);
}

static uint32_t xcompcap_getwidth(void* data)
{
	XCompcapMain* cc = (XCompcapMain*)data;
	return cc->width();
}

static uint32_t xcompcap_getheight(void* data)
{
	XCompcapMain* cc = (XCompcapMain*)data;
	return cc->height();
}

static obs_properties_t xcompcap_props(const char *locale)
{
	return XCompcapMain::properties(locale);
}

void xcompcap_defaults(obs_data_t settings)
{
	XCompcapMain::defaults(settings);
}

void xcompcap_update(void *data, obs_data_t settings)
{
	XCompcapMain* cc = (XCompcapMain*)data;
	cc->updateSettings(settings);
}

OBS_DECLARE_MODULE()

static const char* xcompcap_getname(const char* locale)
{
	UNUSED_PARAMETER(locale);

	return "Xcomposite capture";
}

bool obs_module_load(uint32_t libobs_version)
{
	UNUSED_PARAMETER(libobs_version);

	if (!XCompcapMain::init())
		return false;

	obs_source_info sinfo;
	memset(&sinfo, 0, sizeof(obs_source_info));

	sinfo.id = "xcomposite_input";
	sinfo.output_flags = OBS_SOURCE_VIDEO;

	sinfo.getname      = xcompcap_getname;
	sinfo.create       = xcompcap_create;
	sinfo.destroy      = xcompcap_destroy;
	sinfo.properties   = xcompcap_props;
	sinfo.defaults     = xcompcap_defaults;
	sinfo.update       = xcompcap_update;
	sinfo.video_tick   = xcompcap_video_tick;
	sinfo.video_render = xcompcap_video_render;
	sinfo.getwidth     = xcompcap_getwidth;
	sinfo.getheight    = xcompcap_getheight;

	obs_register_source(&sinfo);

	blog(LOG_INFO, "Xcomposite capture plugin loaded");

	return true;
}

void obs_module_unload()
{
	XCompcapMain::deinit();

	blog(LOG_INFO, "Xcomposite capture plugin unloaded");
}
