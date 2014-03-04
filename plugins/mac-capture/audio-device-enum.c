#include <CoreFoundation/CFString.h>
#include <CoreAudio/CoreAudio.h>

#include "mac-helpers.h"
#include "audio-device-enum.h"

/* ugh, because mac has no means of capturing output, we have to basically
 * mark soundflower and wavtap as output devices. */
static inline bool device_is_input(char *device)
{
	return astrstri(device, "soundflower") == NULL &&
	       astrstri(device, "wavtap")      == NULL;
}

static inline bool enum_success(OSStatus stat, const char *msg)
{
	if (stat != noErr) {
		blog(LOG_WARNING, "[coreaudio_enum_devices] %s failed: %d",
				msg, (int)stat);
		return false;
	}

	return true;
}

static void coreaudio_enum_add_device(struct device_list *list,
		AudioDeviceID id, bool input)
{
	OSStatus           stat;
	UInt32             size     = 0;
	CFStringRef        cf_name  = NULL;
	CFStringRef        cf_value = NULL;
	struct device_item item;

	AudioObjectPropertyAddress addr = {
		kAudioDevicePropertyStreams,
		kAudioDevicePropertyScopeInput,
		kAudioObjectPropertyElementMaster
	};

	memset(&item, 0, sizeof(item));

	/* check to see if it's a mac input device */
	AudioObjectGetPropertyDataSize(id, &addr, 0, NULL, &size);
	if (!size)
		return;

	size = sizeof(CFStringRef);

	addr.mSelector = kAudioDevicePropertyDeviceUID;
	stat = AudioObjectGetPropertyData(id, &addr, 0, NULL, &size, &cf_value);
	if (!enum_success(stat, "get audio device UID"))
		return;

	addr.mSelector = kAudioDevicePropertyDeviceNameCFString;
	stat = AudioObjectGetPropertyData(id, &addr, 0, NULL, &size, &cf_name);
	if (!enum_success(stat, "get audio device name"))
		goto fail;

	if (!cf_to_dstr(cf_name,  &item.name))
		goto fail;
	if (!cf_to_dstr(cf_value, &item.value))
		goto fail;

	if (input || !device_is_input(item.value.array))
		device_list_add(list, &item);

fail:
	device_item_free(&item);
	CFRelease(cf_name);
	CFRelease(cf_value);
}

void coreaudio_enum_devices(struct device_list *list, bool input)
{
	AudioObjectPropertyAddress addr = {
		kAudioHardwarePropertyDevices,
		kAudioObjectPropertyScopeGlobal,
		kAudioObjectPropertyElementMaster
	};

	UInt32        size = 0;
	UInt32        count;
	OSStatus      stat;
	AudioDeviceID *ids;

	stat = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr,
			0, NULL, &size);
	if (!enum_success(stat, "get kAudioObjectSystemObject data size"))
		return;

	ids   = bmalloc(size);
	count = size / sizeof(AudioDeviceID);

	stat = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr,
			0, NULL, &size, ids);

	if (enum_success(stat, "get kAudioObjectSystemObject data"))
		for (UInt32 i = 0; i < count; i++)
			coreaudio_enum_add_device(list, ids[i], input);

	bfree(ids);
}
