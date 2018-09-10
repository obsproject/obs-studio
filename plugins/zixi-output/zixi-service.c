#include <util/platform.h>
#include <obs-module.h>
#include <jansson.h>
#include "include/zixi_feeder_interface.h"

#include "zixi-constants.h"


static const char *zixi_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Push to Zixi Broadcaster");
}


static void zixi_update(void *data, obs_data_t *settings)
{
	struct zixi_settings *service = data;
	char			 zixi_version_str[50];
	int				 maj, mid, min, build;
	zixi_version(&maj, &mid, &min, &build);
	snprintf(zixi_version_str, 50, "%d.%d.%d.%d", maj, mid, min, build);

	obs_data_set_string(settings, "ZixiVersion", zixi_version_str);
	bfree(service->password);
	bfree(service->url);
	bfree(service->encryption_key);
	bfree(service->rtmp_url);
	bfree(service->rtmp_channel);
	bfree(service->rtmp_username);
	bfree(service->rtmp_password);

	obs_data_first(settings);
	service->url = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_URL));
	service->encryption_key = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_ENCRYPTION_KEY));
	service->latency_id = (int)obs_data_get_int(settings, ZIXI_SERVICE_PROP_LATENCY_ID);
	service->encryption_id = (int)obs_data_get_int(settings, ZIXI_SERVICE_PROP_ENCRYPTION_TYPE);
	service->password = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_PASSWORD));
	service->enable_bonding = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_ENABLE_BONDING);

	service->auto_rtmp_out = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_USE_AUTO_RTMP_OUT);
	service->rtmp_url = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_URL));
	service->rtmp_channel = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_CHANNEL));
	service->rtmp_username = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_USERNAME)); 
	service->rtmp_password = bstrdup(obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_PASSWORD));
}

static void zixi_destroy(void *data)
{
	struct zixi_settings *service = data;
	bfree(service->password);
	bfree(service->url);
	bfree(service->encryption_key);
	
	bfree(service->rtmp_url);
	bfree(service->rtmp_channel);
	bfree(service->rtmp_username);
	bfree(service->rtmp_password);

	bfree(service);
}

static void *zixi_create(obs_data_t *settings, obs_service_t *service)
{
	struct zixi_settings *data = bzalloc(sizeof(struct zixi_settings));
	zixi_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static bool bonding_enabled_changed(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings){
	bool new_val = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_ENABLE_BONDING);
	
	return true;
}

static bool latency_changed(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings){
	static unsigned int LATENCIES[] = { 100, 200, 300, 500, 1000, 1500, 2000, 2500, 3000, 4000, 5000 ,6000,8000,10000,12000,14000,16000};
	static const char * LATENCIES_STR[] = { "100 ms", "200 ms", "300 ms", "500 ms", "1000 ms", "1500 ms", "2000 ms", "2500 ms","3000 ms", "4000 ms", "5000 ms", "6000 ms", "8000 ms","10000 ms","12000 ms", "14000 ms", "16000 ms" };
	static unsigned int LATENCY_COUNT = 17;
	obs_property_list_clear(p);

	for (unsigned int i = 0; i < LATENCY_COUNT; i++) {
		obs_property_list_add_int(p, LATENCIES_STR[i], i);
	}

	return true;
}
static bool use_encoder_feedback(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings){

	return true;
}

#ifdef ZIXI_ADVANCED
static bool zixi_advanced_changed(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings){
	bool use = obs_data_get_bool(settings, ZIXI_SERVICE_ADV_USE);
	p = obs_properties_get(ppts, ZIXI_SERVICE_FEC_OVERHEAD);
	obs_property_set_visible(p, use);
	p = obs_properties_get(ppts, ZIXI_SERVICE_FEC_MS);
	obs_property_set_visible(p, use);
	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_ZIXI_LOG_LEVEL);
	obs_property_set_visible(p, use);
//	p = obs_properties_get(ppts, ZIXI_LOW_LATENCY_MUXING);
//	obs_property_set_visible(p, use);
	return true;
}
#endif
static bool use_auto_rtmp_out_changed(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings){
	bool use = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_USE_AUTO_RTMP_OUT);
	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_URL);
	obs_property_set_visible(p, use);
	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_USERNAME);
	obs_property_set_visible(p, use);
	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_PASSWORD);
	obs_property_set_visible(p, use);
	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_CHANNEL);
	obs_property_set_visible(p, use);
	return true;
}
static bool encryption_type_changed(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings){
	static const char * ENCRYPTION_STR[] = {  "AES 128", "AES 192", "AES 256","CHACHA-20","None"};
	static unsigned int ENCRYPTION_COUNT = 5;

	long long t = obs_data_get_int(settings, ZIXI_SERVICE_PROP_ENCRYPTION_TYPE);
	int show_key = t != 4;
	obs_property_list_clear(p);
	for (unsigned int i = 0; i < ENCRYPTION_COUNT; i++) {
		obs_property_list_add_int(p, ENCRYPTION_STR[i], i);
	}

	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_ENCRYPTION_KEY);
	obs_property_set_visible(p, show_key);
	return true;
}

static bool show_all_services_toggled(obs_properties_t *ppts,
	obs_property_t *p, obs_data_t *settings)
{
	int64_t cur_latency_id = obs_data_get_int(settings, ZIXI_SERVICE_PROP_LATENCY_ID);
	bool show_all = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_ENCRYPTION_USE);
	int64_t show_key = obs_data_get_int(settings, ZIXI_SERVICE_PROP_ENCRYPTION_TYPE);

	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_ENCRYPTION_TYPE);
	obs_property_set_visible(p, show_all);
	show_key = show_key != 0 && show_key != 4;

	p = obs_properties_get(ppts, ZIXI_SERVICE_PROP_ENCRYPTION_KEY);
	obs_property_set_visible(p, show_all && show_key);


	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t *zixi_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t   *p;
	p = obs_properties_add_text(ppts, "ZixiVersion", obs_module_text(bstrdup("Version")), OBS_TEXT_DEFAULT);
	obs_property_set_enabled(p, false);
	// obs_property_set_description(p, obs_module_text(bstrdup(zixi_version_str)));

	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_URL, obs_module_text("Zixi URL"),
		OBS_TEXT_DEFAULT);
	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_PASSWORD, obs_module_text("Password"), OBS_TEXT_PASSWORD);

	p = obs_properties_add_list(ppts, ZIXI_SERVICE_PROP_LATENCY_ID, "Latency (ms)", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	latency_changed(ppts,p, NULL);
	// obs_property_set_modified_callback(p, latency_changed);
	p = obs_properties_add_list(ppts, ZIXI_SERVICE_PROP_ENCRYPTION_TYPE, "Encryption type", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_set_modified_callback(p, encryption_type_changed);
	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_ENCRYPTION_KEY, obs_module_text("Encryption key"),
		OBS_TEXT_PASSWORD);

	p = obs_properties_add_bool(ppts, ZIXI_SERVICE_PROP_USE_ENCODER_FEEDBACK, obs_module_text("Use encoder feedback"));
	p = obs_properties_add_bool(ppts, ZIXI_SERVICE_PROP_ENABLE_BONDING, obs_module_text("Enable bonding"));
	// obs_property_set_modified_callback(p, bonding_enabled_changed);
	p = obs_properties_add_bool(ppts, ZIXI_SERVICE_PROP_USE_AUTO_RTMP_OUT, obs_module_text("Forward to RTMP"));
	obs_property_set_modified_callback(p, use_auto_rtmp_out_changed);

	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_URL, obs_module_text("RTMP URL"), OBS_TEXT_DEFAULT);
	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_CHANNEL, obs_module_text("Stream key"), OBS_TEXT_PASSWORD);
	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_USERNAME, obs_module_text("Username"), OBS_TEXT_DEFAULT);
	p = obs_properties_add_text(ppts, ZIXI_SERVICE_PROP_AUTO_RTMP_PASSWORD, obs_module_text("Password"), OBS_TEXT_PASSWORD);
	
	

#ifdef ZIXI_ADVANCED
	p = obs_properties_add_bool(ppts, ZIXI_SERVICE_ADV_USE, obs_module_text("Zixi Advanced Settings"));
	obs_property_set_modified_callback(p, zixi_advanced_changed);
	p = obs_properties_add_int(ppts, ZIXI_SERVICE_FEC_OVERHEAD, obs_module_text("Fec overhead"), 0, 2000, 50);
	obs_property_set_visible(p, false);
	p = obs_properties_add_int(ppts, ZIXI_SERVICE_FEC_MS, obs_module_text("Fec size (ms)"), 0, 300, 25);
	obs_property_set_visible(p, false);
	p = obs_properties_add_int(ppts, ZIXI_SERVICE_PROP_ZIXI_LOG_LEVEL, obs_module_text("Zixi log level"), 0, 5, 1);
	obs_property_set_visible(p, false);
//	p = obs_properties_add_bool(ppts, ZIXI_LOW_LATENCY_MUXING, obs_module_text("Low latency muxing"));
//	obs_property_set_visible(p, false);
#endif
	return ppts;
}

static void zixi_apply_settings(void *data,
	obs_data_t *video_settings, obs_data_t *audio_settings)
{
	struct zixi_settings *service = data;

}

static const char *zixi_url(void *data)
{
	struct zixi_settings *service = data;
	return service->url;
}

static const char *zixi_key(void *data)
{
	struct zixi_settings *service = data;
	return service->encryption_key;
}

static void zixi_get_defaults(obs_data_t* settings) {
	char			 zixi_version_str[50];
	int				 maj, mid, min, build;
	zixi_version(&maj, &mid, &min, &build);
	int ver_len = snprintf(zixi_version_str, 50, "%d.%d.%d.%d", maj, mid, min, build);
	obs_data_set_string(settings, ZIXI_SERVICE_PROP_URL, "zixi://BROADCASTER_HOST:PORT/STREAM");
	obs_data_set_string(settings, "ZixiVersion", zixi_version_str);
	obs_data_set_int(settings, ZIXI_SERVICE_PROP_LATENCY_ID, ZIXI_DEFAULT_LATENCY_ID);
}
static const char * zixi_get_output_type(void * unused) {
	return "zixi_output";
}
struct obs_service_info zixi_service = {
	.id = "zixi_service",
	.get_name = zixi_getname,
	.get_output_type = zixi_get_output_type, 
	.create = zixi_create,
	.destroy = zixi_destroy,
	.update = zixi_update,
	.get_properties = zixi_properties,
	.get_url = zixi_url,
	.get_key = zixi_key,
	.get_defaults = zixi_get_defaults, 
	.apply_encoder_settings = zixi_apply_settings,
};
