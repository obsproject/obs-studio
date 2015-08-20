#include <util/text-lookup.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <file-updater/file-updater.h>

#include "rtmp-format-ver.h"
#include "lookup-config.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("rtmp-services", "en-US")

#define RTMP_SERVICES_LOG_STR "[rtmp-services plugin] "
#define RTMP_SERVICES_VER_STR "rtmp-services plugin (libobs " OBS_VERSION ")"

extern struct obs_service_info rtmp_common_service;
extern struct obs_service_info rtmp_custom_service;

static update_info_t *update_info = NULL;

static bool confirm_service_file(void *param, struct file_download_data *file)
{
	if (astrcmpi(file->name, "services.json") == 0) {
		obs_data_t *data;
		int format_version;

		data = obs_data_create_from_json((char*)file->buffer.array);
		if (!data)
			return false;

		format_version = (int)obs_data_get_int(data, "format_version");
		obs_data_release(data);

		if (format_version != RTMP_SERVICES_FORMAT_VERSION)
			return false;
	}

	UNUSED_PARAMETER(param);
	return true;
}

bool obs_module_load(void)
{
	char *local_dir = obs_module_file("");
	char *cache_dir = obs_module_config_path("");

	if (cache_dir) {
		update_info = update_info_create(
				RTMP_SERVICES_LOG_STR,
				RTMP_SERVICES_VER_STR,
				RTMP_SERVICES_URL,
				local_dir,
				cache_dir,
				confirm_service_file, NULL);
	}

	bfree(local_dir);
	bfree(cache_dir);

	obs_register_service(&rtmp_common_service);
	obs_register_service(&rtmp_custom_service);
	return true;
}

void obs_module_unload(void)
{
	update_info_destroy(update_info);
}
