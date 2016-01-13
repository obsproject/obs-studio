#include "decklink.hpp"
#include "decklink-device.hpp"
#include "decklink-device-discovery.hpp"

#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("decklink", "en-US")

static DeckLinkDeviceDiscovery *deviceEnum = nullptr;

static void decklink_enable_buffering(DeckLink *decklink, bool enabled)
{
	obs_source_t *source = decklink->GetSource();
	uint32_t flags = obs_source_get_flags(source);
	if (enabled)
		flags &= ~OBS_SOURCE_FLAG_UNBUFFERED;
	else
		flags |= OBS_SOURCE_FLAG_UNBUFFERED;
	obs_source_set_flags(source, flags);
}

static void *decklink_create(obs_data_t *settings, obs_source_t *source)
{
	DeckLink *decklink = new DeckLink(source, deviceEnum);

	decklink_enable_buffering(decklink,
			obs_data_get_bool(settings, "buffering"));

	obs_source_update(source, settings);
	return decklink;
}

static void decklink_destroy(void *data)
{
	DeckLink *decklink = (DeckLink *)data;
	delete decklink;
}

static void decklink_update(void *data, obs_data_t *settings)
{
	DeckLink *decklink = (DeckLink *)data;
	const char *hash = obs_data_get_string(settings, "device_hash");
	long long id = obs_data_get_int(settings, "mode_id");
	BMDPixelFormat format = (BMDPixelFormat)obs_data_get_int(settings,
			"pixel_format");

	decklink_enable_buffering(decklink,
			obs_data_get_bool(settings, "buffering"));

	ComPtr<DeckLinkDevice> device;
	device.Set(deviceEnum->FindByHash(hash));

	decklink->SetPixelFormat(format);
	decklink->Activate(device, id);
}

static void decklink_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "buffering", true);
	obs_data_set_default_int(settings, "pixel_format", bmdFormat8BitYUV);
}

static const char *decklink_get_name(void*)
{
	return obs_module_text("BlackmagicDevice");
}

static bool decklink_device_changed(obs_properties_t *props,
		obs_property_t *list, obs_data_t *settings)
{
	const char *name = obs_data_get_string(settings, "device_name");
	const char *hash = obs_data_get_string(settings, "device_hash");
	const char *mode = obs_data_get_string(settings, "mode_name");
	long long modeId = obs_data_get_int(settings, "mode_id");

	size_t itemCount = obs_property_list_item_count(list);
	bool itemFound = false;

	for (size_t i = 0; i < itemCount; i++) {
		const char *curHash = obs_property_list_item_string(list, i);
		if (strcmp(hash, curHash) == 0) {
			itemFound = true;
			break;
		}
	}

	if (!itemFound) {
		obs_property_list_insert_string(list, 0, name, hash);
		obs_property_list_item_disable(list, 0, true);
	}

	list = obs_properties_get(props, "mode_id");

	obs_property_list_clear(list);

	ComPtr<DeckLinkDevice> device;
	device.Set(deviceEnum->FindByHash(hash));

	if (!device) {
		obs_property_list_add_int(list, mode, modeId);
		obs_property_list_item_disable(list, 0, true);
	} else {
		const std::vector<DeckLinkDeviceMode*> &modes =
			device->GetModes();

		for (DeckLinkDeviceMode *mode : modes) {
			obs_property_list_add_int(list,
					mode->GetName().c_str(),
					mode->GetId());
		}
	}

	return true;
}

static void fill_out_devices(obs_property_t *list)
{
	deviceEnum->Lock();

	const std::vector<DeckLinkDevice*> &devices = deviceEnum->GetDevices();
	for (DeckLinkDevice *device : devices) {
		obs_property_list_add_string(list,
				device->GetDisplayName().c_str(),
				device->GetHash().c_str());
	}

	deviceEnum->Unlock();
}

static obs_properties_t *decklink_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *list = obs_properties_add_list(props, "device_hash",
			obs_module_text("Device"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);
	obs_property_set_modified_callback(list, decklink_device_changed);

	fill_out_devices(list);

	list = obs_properties_add_list(props, "mode_id",
			obs_module_text("Mode"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);

	list = obs_properties_add_list(props, "pixel_format",
			obs_module_text("PixelFormat"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "8-bit YUV", bmdFormat8BitYUV);
	obs_property_list_add_int(list, "8-bit BGRA", bmdFormat8BitBGRA);

	obs_properties_add_bool(props, "buffering",
			obs_module_text("Buffering"));

	UNUSED_PARAMETER(data);
	return props;
}

bool obs_module_load(void)
{
	deviceEnum = new DeckLinkDeviceDiscovery();
	if (!deviceEnum->Init())
		return true;

	struct obs_source_info info = {};
	info.id             = "decklink-input";
	info.type           = OBS_SOURCE_TYPE_INPUT;
	info.output_flags   = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO |
	                      OBS_SOURCE_DO_NOT_DUPLICATE;
	info.create         = decklink_create;
	info.destroy        = decklink_destroy;
	info.get_defaults   = decklink_get_defaults;
	info.get_name       = decklink_get_name;
	info.get_properties = decklink_get_properties;
	info.update         = decklink_update;

	obs_register_source(&info);

	return true;
}

void obs_module_unload(void)
{
	delete deviceEnum;
}
