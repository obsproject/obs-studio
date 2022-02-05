#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CFString.h>
#include <Foundation/Foundation.h>
#include <CoreAudio/CoreAudio.h>
#include <unistd.h>
#include <errno.h>

#include <obs-module.h>
#include <mach/mach_time.h>
#include <util/threading.h>
#include <util/c99defs.h>
#include <util/apple/cfstring-utils.h>

#include "audio-device-enum.h"

#define PROPERTY_DEFAULT_DEVICE kAudioHardwarePropertyDefaultInputDevice
#define PROPERTY_FORMATS kAudioStreamPropertyAvailablePhysicalFormats

#define SCOPE_OUTPUT kAudioUnitScope_Output
#define SCOPE_INPUT kAudioUnitScope_Input
#define SCOPE_GLOBAL kAudioUnitScope_Global

#define BUS_OUTPUT 0
#define BUS_INPUT 1

#define MAX_DEVICES 20

#define set_property AudioUnitSetProperty
#define get_property AudioUnitGetProperty

#define TEXT_AUDIO_INPUT obs_module_text("CoreAudio.InputCapture");
#define TEXT_AUDIO_OUTPUT obs_module_text("CoreAudio.OutputCapture");
#define TEXT_DEVICE obs_module_text("CoreAudio.Device")
#define TEXT_DEVICE_DEFAULT obs_module_text("CoreAudio.Device.Default")
#define TEXT_DEVICE_MAC_DESKTOP_CAPTURE \
	obs_module_text("MacAudio.DeviceName.HAL")

#define DESKTOP_CAPTURE_HAL_DEVICE_UID "ODACDevice_UID"
#define DESKTOP_CAPTURE_MULTIOUTPUT_DEVICE_NAME \
	obs_module_text("MacAudio.DeviceName.MultiOutput")
#define DESKTOP_CAPTURE_MULTIOUTPUT_DEVICE_UID "OBSDesktopAudioMultioutput_UID"

struct coreaudio_data {
	char *device_name;
	char *device_uid;
	AudioUnit unit;
	AudioDeviceID device_id;
	AudioBufferList *buf_list;
	bool au_initialized;
	bool active;
	bool default_device;
	bool input;
	bool no_devices;

	uint32_t sample_rate;
	enum audio_format format;
	enum speaker_layout speakers;

	pthread_t reconnect_thread;
	os_event_t *exit_event;
	volatile bool reconnecting;
	unsigned long retry_time;

	obs_source_t *source;
};

static bool get_default_output_device(struct coreaudio_data *ca)
{
	struct device_list list;

	memset(&list, 0, sizeof(struct device_list));
	coreaudio_enum_devices(&list, false);

	if (!list.items.num)
		return false;

	bfree(ca->device_uid);
	ca->device_uid = bstrdup(list.items.array[0].value.array);

	device_list_free(&list);
	return true;
}

static bool find_device_id_by_uid(struct coreaudio_data *ca)
{
	UInt32 size = sizeof(AudioDeviceID);
	CFStringRef cf_uid = NULL;
	CFStringRef qual = NULL;
	UInt32 qual_size = 0;
	OSStatus stat;
	bool success;

	AudioObjectPropertyAddress addr = {
		.mScope = kAudioObjectPropertyScopeGlobal,
		.mElement = kAudioObjectPropertyElementMaster};

	if (!ca->device_uid)
		ca->device_uid = bstrdup("default");

	ca->default_device = false;
	ca->no_devices = false;

	/* have to do this because mac output devices don't actually exist */
	if (astrcmpi(ca->device_uid, "default") == 0) {
		if (ca->input) {
			ca->default_device = true;
		} else {
			if (!get_default_output_device(ca)) {
				ca->no_devices = true;
				return false;
			}
		}
	}

	cf_uid = CFStringCreateWithCString(NULL, ca->device_uid,
					   kCFStringEncodingUTF8);

	if (ca->default_device) {
		addr.mSelector = PROPERTY_DEFAULT_DEVICE;
		stat = AudioObjectGetPropertyData(kAudioObjectSystemObject,
						  &addr, qual_size, &qual,
						  &size, &ca->device_id);
		success = (stat == noErr);
	} else {
		success = coreaudio_get_device_id(cf_uid, &ca->device_id);
	}

	if (cf_uid)
		CFRelease(cf_uid);

	return success;
}

static inline void ca_warn(struct coreaudio_data *ca, const char *func,
			   const char *format, ...)
{
	va_list args;
	struct dstr str = {0};

	va_start(args, format);

	dstr_printf(&str, "[%s]:[device '%s'] ", func, ca->device_name);
	dstr_vcatf(&str, format, args);
	blog(LOG_WARNING, "%s", str.array);
	dstr_free(&str);

	va_end(args);
}

static inline bool ca_success(OSStatus stat, struct coreaudio_data *ca,
			      const char *func, const char *action)
{
	if (stat != noErr) {
		blog(LOG_WARNING, "[%s]:[device '%s'] %s failed: %d", func,
		     ca->device_name, action, (int)stat);
		return false;
	}

	return true;
}

enum coreaudio_io_type {
	IO_TYPE_INPUT,
	IO_TYPE_OUTPUT,
};

static inline bool enable_io(struct coreaudio_data *ca,
			     enum coreaudio_io_type type, bool enable)
{
	UInt32 enable_int = enable;
	return set_property(ca->unit, kAudioOutputUnitProperty_EnableIO,
			    (type == IO_TYPE_INPUT) ? SCOPE_INPUT
						    : SCOPE_OUTPUT,
			    (type == IO_TYPE_INPUT) ? BUS_INPUT : BUS_OUTPUT,
			    &enable_int, sizeof(enable_int));
}

static inline enum audio_format convert_ca_format(UInt32 format_flags,
						  UInt32 bits)
{
	bool planar = (format_flags & kAudioFormatFlagIsNonInterleaved) != 0;

	if (format_flags & kAudioFormatFlagIsFloat)
		return planar ? AUDIO_FORMAT_FLOAT_PLANAR : AUDIO_FORMAT_FLOAT;

	if (!(format_flags & kAudioFormatFlagIsSignedInteger) && bits == 8)
		return planar ? AUDIO_FORMAT_U8BIT_PLANAR : AUDIO_FORMAT_U8BIT;

	/* not float?  not signed int?  no clue, fail */
	if ((format_flags & kAudioFormatFlagIsSignedInteger) == 0)
		return AUDIO_FORMAT_UNKNOWN;

	if (bits == 16)
		return planar ? AUDIO_FORMAT_16BIT_PLANAR : AUDIO_FORMAT_16BIT;
	else if (bits == 32)
		return planar ? AUDIO_FORMAT_32BIT_PLANAR : AUDIO_FORMAT_32BIT;

	return AUDIO_FORMAT_UNKNOWN;
}

static inline enum speaker_layout convert_ca_speaker_layout(UInt32 channels)
{
	switch (channels) {
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	}

	return SPEAKERS_UNKNOWN;
}

static bool coreaudio_init_format(struct coreaudio_data *ca)
{
	AudioStreamBasicDescription desc;
	OSStatus stat;
	UInt32 size = sizeof(desc);
	struct obs_audio_info aoi;
	int channels;

	if (!obs_get_audio_info(&aoi)) {
		blog(LOG_WARNING, "No active audio");
		return false;
	}
	channels = get_audio_channels(aoi.speakers);

	stat = get_property(ca->unit, kAudioUnitProperty_StreamFormat,
			    SCOPE_INPUT, BUS_INPUT, &desc, &size);
	if (!ca_success(stat, ca, "coreaudio_init_format", "get input format"))
		return false;

	/* Certain types of devices have no limit on channel count, and
	 * there's no way to know the actual number of channels it's using,
	 * so if we encounter this situation just force to what is defined in output */
	if (desc.mChannelsPerFrame > 8) {
		desc.mChannelsPerFrame = channels;
		desc.mBytesPerFrame = channels * desc.mBitsPerChannel / 8;
		desc.mBytesPerPacket =
			desc.mFramesPerPacket * desc.mBytesPerFrame;
	}

	stat = set_property(ca->unit, kAudioUnitProperty_StreamFormat,
			    SCOPE_OUTPUT, BUS_INPUT, &desc, size);
	if (!ca_success(stat, ca, "coreaudio_init_format", "set output format"))
		return false;

	if (desc.mFormatID != kAudioFormatLinearPCM) {
		ca_warn(ca, "coreaudio_init_format", "format is not PCM");
		return false;
	}

	ca->format = convert_ca_format(desc.mFormatFlags, desc.mBitsPerChannel);
	if (ca->format == AUDIO_FORMAT_UNKNOWN) {
		ca_warn(ca, "coreaudio_init_format",
			"unknown format flags: "
			"%u, bits: %u",
			(unsigned int)desc.mFormatFlags,
			(unsigned int)desc.mBitsPerChannel);
		return false;
	}

	ca->sample_rate = (uint32_t)desc.mSampleRate;
	ca->speakers = convert_ca_speaker_layout(desc.mChannelsPerFrame);

	if (ca->speakers == SPEAKERS_UNKNOWN) {
		ca_warn(ca, "coreaudio_init_format",
			"unknown speaker layout: "
			"%u channels",
			(unsigned int)desc.mChannelsPerFrame);
		return false;
	}

	return true;
}

static bool coreaudio_init_buffer(struct coreaudio_data *ca)
{
	UInt32 buf_size = 0;
	UInt32 size = 0;
	UInt32 frames = 0;
	OSStatus stat;

	AudioObjectPropertyAddress addr = {
		kAudioDevicePropertyStreamConfiguration,
		kAudioDevicePropertyScopeInput,
		kAudioObjectPropertyElementMaster};

	stat = AudioObjectGetPropertyDataSize(ca->device_id, &addr, 0, NULL,
					      &buf_size);
	if (!ca_success(stat, ca, "coreaudio_init_buffer", "get list size"))
		return false;

	size = sizeof(frames);
	stat = get_property(ca->unit, kAudioDevicePropertyBufferFrameSize,
			    SCOPE_GLOBAL, 0, &frames, &size);
	if (!ca_success(stat, ca, "coreaudio_init_buffer", "get frame size"))
		return false;

	/* ---------------------- */

	ca->buf_list = bmalloc(buf_size);

	stat = AudioObjectGetPropertyData(ca->device_id, &addr, 0, NULL,
					  &buf_size, ca->buf_list);
	if (!ca_success(stat, ca, "coreaudio_init_buffer", "allocate")) {
		bfree(ca->buf_list);
		ca->buf_list = NULL;
		return false;
	}

	for (UInt32 i = 0; i < ca->buf_list->mNumberBuffers; i++) {
		size = ca->buf_list->mBuffers[i].mDataByteSize;
		ca->buf_list->mBuffers[i].mData = bmalloc(size);
	}

	return true;
}

static void buf_list_free(AudioBufferList *buf_list)
{
	if (buf_list) {
		for (UInt32 i = 0; i < buf_list->mNumberBuffers; i++)
			bfree(buf_list->mBuffers[i].mData);

		bfree(buf_list);
	}
}

static OSStatus input_callback(void *data,
			       AudioUnitRenderActionFlags *action_flags,
			       const AudioTimeStamp *ts_data, UInt32 bus_num,
			       UInt32 frames, AudioBufferList *ignored_buffers)
{
	struct coreaudio_data *ca = data;
	OSStatus stat;
	struct obs_source_audio audio;

	stat = AudioUnitRender(ca->unit, action_flags, ts_data, bus_num, frames,
			       ca->buf_list);
	if (!ca_success(stat, ca, "input_callback", "audio retrieval"))
		return noErr;

	for (UInt32 i = 0; i < ca->buf_list->mNumberBuffers; i++)
		audio.data[i] = ca->buf_list->mBuffers[i].mData;

	audio.frames = frames;
	audio.speakers = ca->speakers;
	audio.format = ca->format;
	audio.samples_per_sec = ca->sample_rate;
	static double factor = 0.;
	static mach_timebase_info_data_t info = {0, 0};
	if (info.numer == 0 && info.denom == 0) {
		mach_timebase_info(&info);
		factor = ((double)info.numer) / info.denom;
	}
	if (info.numer != info.denom)
		audio.timestamp = (uint64_t)(factor * ts_data->mHostTime);
	else
		audio.timestamp = ts_data->mHostTime;

	obs_source_output_audio(ca->source, &audio);

	UNUSED_PARAMETER(ignored_buffers);
	return noErr;
}

static void coreaudio_stop(struct coreaudio_data *ca);
static bool coreaudio_init(struct coreaudio_data *ca);
static void coreaudio_uninit(struct coreaudio_data *ca);

static void *reconnect_thread(void *param)
{
	struct coreaudio_data *ca = param;

	ca->reconnecting = true;

	while (os_event_timedwait(ca->exit_event, ca->retry_time) ==
	       ETIMEDOUT) {
		if (coreaudio_init(ca))
			break;
	}

	blog(LOG_DEBUG, "coreaudio: exit the reconnect thread");
	ca->reconnecting = false;
	return NULL;
}

static void coreaudio_begin_reconnect(struct coreaudio_data *ca)
{
	int ret;

	if (ca->reconnecting)
		return;

	ret = pthread_create(&ca->reconnect_thread, NULL, reconnect_thread, ca);
	if (ret != 0)
		blog(LOG_WARNING,
		     "[coreaudio_begin_reconnect] failed to "
		     "create thread, error code: %d",
		     ret);
}

static OSStatus
notification_callback(AudioObjectID id, UInt32 num_addresses,
		      const AudioObjectPropertyAddress addresses[], void *data)
{
	struct coreaudio_data *ca = data;

	coreaudio_stop(ca);
	coreaudio_uninit(ca);

	if (addresses[0].mSelector == PROPERTY_DEFAULT_DEVICE)
		ca->retry_time = 300;
	else
		ca->retry_time = 2000;

	blog(LOG_INFO,
	     "coreaudio: device '%s' disconnected or changed.  "
	     "attempting to reconnect",
	     ca->device_name);

	coreaudio_begin_reconnect(ca);

	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(num_addresses);

	return noErr;
}

static OSStatus add_listener(struct coreaudio_data *ca, UInt32 property)
{
	AudioObjectPropertyAddress addr = {property,
					   kAudioObjectPropertyScopeGlobal,
					   kAudioObjectPropertyElementMaster};

	return AudioObjectAddPropertyListener(ca->device_id, &addr,
					      notification_callback, ca);
}

static bool coreaudio_init_hooks(struct coreaudio_data *ca)
{
	OSStatus stat;
	AURenderCallbackStruct callback_info = {.inputProc = input_callback,
						.inputProcRefCon = ca};

	stat = add_listener(ca, kAudioDevicePropertyDeviceIsAlive);
	if (!ca_success(stat, ca, "coreaudio_init_hooks",
			"set disconnect callback"))
		return false;

	stat = add_listener(ca, PROPERTY_FORMATS);
	if (!ca_success(stat, ca, "coreaudio_init_hooks",
			"set format change callback"))
		return false;

	if (ca->default_device) {
		AudioObjectPropertyAddress addr = {
			PROPERTY_DEFAULT_DEVICE,
			kAudioObjectPropertyScopeGlobal,
			kAudioObjectPropertyElementMaster};

		stat = AudioObjectAddPropertyListener(kAudioObjectSystemObject,
						      &addr,
						      notification_callback,
						      ca);
		if (!ca_success(stat, ca, "coreaudio_init_hooks",
				"set device change callback"))
			return false;
	}

	stat = set_property(ca->unit, kAudioOutputUnitProperty_SetInputCallback,
			    SCOPE_GLOBAL, 0, &callback_info,
			    sizeof(callback_info));
	if (!ca_success(stat, ca, "coreaudio_init_hooks", "set input callback"))
		return false;

	return true;
}

static void coreaudio_remove_hooks(struct coreaudio_data *ca)
{
	AURenderCallbackStruct callback_info = {.inputProc = NULL,
						.inputProcRefCon = NULL};

	AudioObjectPropertyAddress addr = {kAudioDevicePropertyDeviceIsAlive,
					   kAudioObjectPropertyScopeGlobal,
					   kAudioObjectPropertyElementMaster};

	AudioObjectRemovePropertyListener(ca->device_id, &addr,
					  notification_callback, ca);

	addr.mSelector = PROPERTY_FORMATS;
	AudioObjectRemovePropertyListener(ca->device_id, &addr,
					  notification_callback, ca);

	if (ca->default_device) {
		addr.mSelector = PROPERTY_DEFAULT_DEVICE;
		AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
						  &addr, notification_callback,
						  ca);
	}

	set_property(ca->unit, kAudioOutputUnitProperty_SetInputCallback,
		     SCOPE_GLOBAL, 0, &callback_info, sizeof(callback_info));
}

static bool coreaudio_get_device_name(struct coreaudio_data *ca)
{
	CFStringRef cf_name = NULL;
	UInt32 size = sizeof(CFStringRef);
	char *name = NULL;

	const AudioObjectPropertyAddress addr = {
		kAudioDevicePropertyDeviceNameCFString,
		kAudioObjectPropertyScopeInput,
		kAudioObjectPropertyElementMaster};

	OSStatus stat = AudioObjectGetPropertyData(ca->device_id, &addr, 0,
						   NULL, &size, &cf_name);
	if (stat != noErr) {
		blog(LOG_WARNING,
		     "[coreaudio_get_device_name] failed to "
		     "get name: %d",
		     (int)stat);
		return false;
	}

	name = cfstr_copy_cstr(cf_name, kCFStringEncodingUTF8);
	if (!name) {
		blog(LOG_WARNING, "[coreaudio_get_device_name] failed to "
				  "convert name to cstr for some reason");
		return false;
	}

	bfree(ca->device_name);
	ca->device_name = name;

	if (cf_name)
		CFRelease(cf_name);

	return true;
}

static bool coreaudio_start(struct coreaudio_data *ca)
{
	OSStatus stat;

	if (ca->active)
		return true;

	stat = AudioOutputUnitStart(ca->unit);
	return ca_success(stat, ca, "coreaudio_start", "start audio");
}

static void coreaudio_stop(struct coreaudio_data *ca)
{
	OSStatus stat;

	if (!ca->active)
		return;

	ca->active = false;

	stat = AudioOutputUnitStop(ca->unit);
	ca_success(stat, ca, "coreaudio_stop", "stop audio");
}

static bool coreaudio_init_unit(struct coreaudio_data *ca)
{
	AudioComponentDescription desc = {
		.componentType = kAudioUnitType_Output,
		.componentSubType = kAudioUnitSubType_HALOutput};

	AudioComponent component = AudioComponentFindNext(NULL, &desc);
	if (!component) {
		ca_warn(ca, "coreaudio_init_unit", "find component failed");
		return false;
	}

	OSStatus stat = AudioComponentInstanceNew(component, &ca->unit);
	if (!ca_success(stat, ca, "coreaudio_init_unit", "instance unit"))
		return false;

	ca->au_initialized = true;
	return true;
}

static bool coreaudio_init(struct coreaudio_data *ca)
{
	OSStatus stat;

	if (ca->au_initialized)
		return true;

	if (!find_device_id_by_uid(ca))
		return false;
	if (!coreaudio_get_device_name(ca))
		return false;
	if (!coreaudio_init_unit(ca))
		return false;

	stat = enable_io(ca, IO_TYPE_INPUT, true);
	if (!ca_success(stat, ca, "coreaudio_init", "enable input io"))
		goto fail;

	stat = enable_io(ca, IO_TYPE_OUTPUT, false);
	if (!ca_success(stat, ca, "coreaudio_init", "disable output io"))
		goto fail;

	stat = set_property(ca->unit, kAudioOutputUnitProperty_CurrentDevice,
			    SCOPE_GLOBAL, 0, &ca->device_id,
			    sizeof(ca->device_id));
	if (!ca_success(stat, ca, "coreaudio_init", "set current device"))
		goto fail;

	if (!coreaudio_init_format(ca))
		goto fail;
	if (!coreaudio_init_buffer(ca))
		goto fail;
	if (!coreaudio_init_hooks(ca))
		goto fail;

	stat = AudioUnitInitialize(ca->unit);
	if (!ca_success(stat, ca, "coreaudio_initialize", "initialize"))
		goto fail;

	if (!coreaudio_start(ca))
		goto fail;

	blog(LOG_INFO, "coreaudio: device '%s' initialized", ca->device_name);
	return ca->au_initialized;

fail:
	coreaudio_uninit(ca);
	return false;
}

static NSString *halPluginFileName =
	@"/Library/Audio/Plug-Ins/HAL/OBSDesktopAudioCapture.driver";

static bool is_hal_device_updated()
{
	NSString *obsVersion = [[[NSBundle mainBundle] infoDictionary]
		objectForKey:@"CFBundleShortVersionString"];
	NSString *obsBuild = [[[NSBundle mainBundle] infoDictionary]
		objectForKey:(NSString *)kCFBundleVersionKey];

	// Can't use [NSBundle infoDictionary] here since that
	// appears to be cached, which is unhelpful if you're
	// trying to find out if something changed.
	NSError *ignored;
	NSURL *url = [NSURL
		fileURLWithPath:
			[NSString stringWithFormat:@"%@/Contents/Info.plist",
						   halPluginFileName]];
	NSDictionary *halPlist =
		[NSDictionary dictionaryWithContentsOfURL:url error:&ignored];
	NSString *halVersion =
		[halPlist objectForKey:@"CFBundleShortVersionString"];
	NSString *halBuild =
		[halPlist objectForKey:(NSString *)kCFBundleVersionKey];

	return [halVersion isEqualToString:obsVersion] &&
	       [halBuild isEqualToString:obsBuild];
}

static bool is_hal_device_installed()
{
	AudioDeviceID deviceId;
	coreaudio_get_device_id(CFSTR(DESKTOP_CAPTURE_HAL_DEVICE_UID),
				&deviceId);
	return deviceId != 0;
}

static bool modify_hal_plugin(bool reinstall)
{
	NSString *removeCmd = [NSString
		stringWithFormat:@"rm -rf '%@' && ", halPluginFileName];
	NSString *installCmd;
	if (reinstall) {
		NSString *halPluginSourcePath = [[NSBundle mainBundle]
			pathForResource:@"OBSDesktopAudioCapture.driver"
				 ofType:nil];
		NSString *halPluginDestinationPath =
			@"/Library/Audio/Plug-Ins/HAL/";

		NSString *createPluginDirCmd =
			(![[NSFileManager defaultManager]
				fileExistsAtPath:halPluginDestinationPath])
				? [NSString stringWithFormat:
						    @"mkdir -p '%@' && ",
						    halPluginDestinationPath]
				: @"";

		installCmd =
			[NSString stringWithFormat:@"%@cp -R '%@' '%@' && ",
						   createPluginDirCmd,
						   halPluginSourcePath,
						   halPluginDestinationPath];
	} else {
		installCmd = @"";
	}
	NSString *restartCmd =
		@"launchctl kickstart -k system/com.apple.audio.coreaudiod";
	NSString *appleScriptString = [NSString
		stringWithFormat:
			@"do shell script \"%@%@%@\" with administrator privileges",
			removeCmd, installCmd, restartCmd];

	NSAppleScript *appleScriptObject =
		[[NSAppleScript alloc] initWithSource:appleScriptString];
	NSDictionary *error;
	NSAppleEventDescriptor *eventDescriptor =
		[appleScriptObject executeAndReturnError:&error];
	if (eventDescriptor == nil) {
		const char *errorMessage = [[error
			objectForKey:@"NSAppleScriptErrorMessage"] UTF8String];
		blog(LOG_WARNING,
		     "[mac-capture]: Failed to modify HAL plugin: %s",
		     errorMessage);
		return false;
	}

	// Hack to wait until CoreAudio has reloaded. I hate it too.
	sleep(5);

	return true;
}

static bool is_multioutput_device_installed()
{
	AudioDeviceID deviceId;
	coreaudio_get_device_id(CFSTR(DESKTOP_CAPTURE_MULTIOUTPUT_DEVICE_UID),
				&deviceId);
	return deviceId != 0;
}

static bool delete_multioutput_device()
{
	AudioDeviceID deviceId;
	coreaudio_get_device_id(CFSTR(DESKTOP_CAPTURE_MULTIOUTPUT_DEVICE_UID),
				&deviceId);

	if (deviceId) {
		/* Delete device */
		OSStatus status = AudioHardwareDestroyAggregateDevice(deviceId);
		if (status != noErr) {
			blog(LOG_INFO, "Deleting old device failed. Reason: %d",
			     status);
			return false;
		}
		return true;
	}
	return false;
}

static inline bool get_default_monitoring_output_device(CFStringRef *cf_uid)
{
	AudioObjectPropertyAddress addr = {
		kAudioHardwarePropertyDefaultOutputDevice,
		kAudioObjectPropertyScopeOutput,
		kAudioObjectPropertyElementMaster};

	AudioDeviceID id = 0;
	UInt32 size = sizeof(id);
	OSStatus stat = AudioObjectGetPropertyData(kAudioObjectSystemObject,
						   &addr, 0, NULL, &size, &id);

	if (stat != noErr)
		return false;

	size = sizeof(CFStringRef);
	addr.mSelector = kAudioDevicePropertyDeviceUID;
	stat = AudioObjectGetPropertyData(id, &addr, 0, NULL, &size, cf_uid);

	return stat == noErr;
}

static bool create_multioutput_device()
{
	CFMutableDictionaryRef params =
		CFDictionaryCreateMutable(kCFAllocatorDefault, 10, NULL, NULL);

	CFDictionaryAddValue(params, CFSTR(kAudioAggregateDeviceUIDKey),
			     CFSTR(DESKTOP_CAPTURE_MULTIOUTPUT_DEVICE_UID));
	CFDictionaryAddValue(params, CFSTR(kAudioAggregateDeviceNameKey),
			     CFStringCreateWithCString(
				     kCFAllocatorDefault,
				     DESKTOP_CAPTURE_MULTIOUTPUT_DEVICE_NAME,
				     kTextEncodingDefaultFormat));

	int stacked = 1;
	CFNumberRef cfStacked =
		CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &stacked);
	CFDictionaryAddValue(params, CFSTR(kAudioAggregateDeviceIsStackedKey),
			     cfStacked);

	/* Add subdevices to aggregate device */
	{
		CFMutableArrayRef subDevices =
			CFArrayCreateMutable(kCFAllocatorDefault, 2, NULL);

		/* Add OBS private cable */
		CFMutableDictionaryRef virtualCableDevice =
			CFDictionaryCreateMutable(kCFAllocatorDefault, 10, NULL,
						  NULL);
		CFDictionaryAddValue(virtualCableDevice,
				     CFSTR(kAudioSubDeviceUIDKey),
				     CFSTR(DESKTOP_CAPTURE_HAL_DEVICE_UID));
		CFArrayAppendValue(subDevices, virtualCableDevice);

		/* Add user-default output device */
		CFStringRef userMainOutputDeviceUid;
		if (!get_default_monitoring_output_device(
			    &userMainOutputDeviceUid))
			blog(LOG_WARNING,
			     "[mac-capture]: Failed to find default output device.");
		if (userMainOutputDeviceUid != NULL) {
			CFMutableDictionaryRef mainDevice =
				CFDictionaryCreateMutable(kCFAllocatorDefault,
							  10, NULL, NULL);
			CFDictionaryAddValue(mainDevice,
					     CFSTR(kAudioSubDeviceUIDKey),
					     userMainOutputDeviceUid);
			CFArrayAppendValue(subDevices, mainDevice);
		} else {
			blog(LOG_WARNING,
			     "[mac-capture]: No audio output device found. Not setting subDevice in aggregate.");
		}

		CFDictionaryAddValue(
			params, CFSTR(kAudioAggregateDeviceSubDeviceListKey),
			subDevices);
	}

	AudioDeviceID resulting_id = 0;
	OSStatus result =
		AudioHardwareCreateAggregateDevice(params, &resulting_id);

	if (result != noErr) {
		blog(LOG_WARNING,
		     "[mac-capture]: Error while creating multi output audio device: %d",
		     result);
		return false;
	}
	return true;
}

static void coreaudio_try_init(struct coreaudio_data *ca)
{
	if (!coreaudio_init(ca)) {
		blog(LOG_INFO,
		     "coreaudio: failed to find device "
		     "uid: %s, waiting for connection",
		     ca->device_uid);

		ca->retry_time = 2000;

		if (ca->no_devices)
			blog(LOG_INFO, "coreaudio: no device found");
		else
			coreaudio_begin_reconnect(ca);
	}
}

static void coreaudio_uninit(struct coreaudio_data *ca)
{
	if (!ca->au_initialized)
		return;

	if (ca->unit) {
		coreaudio_stop(ca);

		OSStatus stat = AudioUnitUninitialize(ca->unit);
		ca_success(stat, ca, "coreaudio_uninit", "uninitialize");

		coreaudio_remove_hooks(ca);

		stat = AudioComponentInstanceDispose(ca->unit);
		ca_success(stat, ca, "coreaudio_uninit", "dispose");

		ca->unit = NULL;
	}

	ca->au_initialized = false;

	buf_list_free(ca->buf_list);
	ca->buf_list = NULL;
}

/* ------------------------------------------------------------------------- */

static const char *coreaudio_input_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_AUDIO_INPUT;
}

static const char *coreaudio_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_AUDIO_OUTPUT;
}

static void coreaudio_shutdown(struct coreaudio_data *ca)
{
	if (ca->reconnecting) {
		os_event_signal(ca->exit_event);
		pthread_join(ca->reconnect_thread, NULL);
		os_event_reset(ca->exit_event);
	}

	coreaudio_uninit(ca);

	if (ca->unit)
		AudioComponentInstanceDispose(ca->unit);
}

static void coreaudio_destroy(void *data)
{
	struct coreaudio_data *ca = data;

	if (ca) {
		coreaudio_shutdown(ca);

		os_event_destroy(ca->exit_event);
		bfree(ca->device_name);
		bfree(ca->device_uid);
		bfree(ca);
	}
}

static void coreaudio_update(void *data, obs_data_t *settings)
{
	struct coreaudio_data *ca = data;

	coreaudio_shutdown(ca);

	bfree(ca->device_uid);
	ca->device_uid = bstrdup(obs_data_get_string(settings, "device_id"));

	coreaudio_try_init(ca);
}

static void coreaudio_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "device_id", "default");
}

static void *coreaudio_create(obs_data_t *settings, obs_source_t *source,
			      bool input)
{
	struct coreaudio_data *ca = bzalloc(sizeof(struct coreaudio_data));

	if (os_event_init(&ca->exit_event, OS_EVENT_TYPE_MANUAL) != 0) {
		blog(LOG_ERROR,
		     "[coreaudio_create] failed to create "
		     "semephore: %d",
		     errno);
		bfree(ca);
		return NULL;
	}

	ca->device_uid = bstrdup(obs_data_get_string(settings, "device_id"));
	ca->source = source;
	ca->input = input;

	if (!ca->device_uid)
		ca->device_uid = bstrdup("default");

	coreaudio_try_init(ca);
	return ca;
}

static void *coreaudio_create_input_capture(obs_data_t *settings,
					    obs_source_t *source)
{
	return coreaudio_create(settings, source, true);
}

static void *coreaudio_create_output_capture(obs_data_t *settings,
					     obs_source_t *source)
{
	return coreaudio_create(settings, source, false);
}

static obs_properties_t *coreaudio_properties(bool input)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *property;
	struct device_list devices;

	memset(&devices, 0, sizeof(struct device_list));

	property = obs_properties_add_list(props, "device_id", TEXT_DEVICE,
					   OBS_COMBO_TYPE_LIST,
					   OBS_COMBO_FORMAT_STRING);

	coreaudio_enum_devices(&devices, input);

	if (!input && is_hal_device_installed())
		obs_property_list_add_string(property,
					     TEXT_DEVICE_MAC_DESKTOP_CAPTURE,
					     DESKTOP_CAPTURE_HAL_DEVICE_UID);

	if (devices.items.num)
		obs_property_list_add_string(property, TEXT_DEVICE_DEFAULT,
					     "default");

	for (size_t i = 0; i < devices.items.num; i++) {
		struct device_item *item = devices.items.array + i;
		obs_property_list_add_string(property, item->name.array,
					     item->value.array);
	}

	device_list_free(&devices);
	return props;
}

static obs_properties_t *coreaudio_input_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	return coreaudio_properties(true);
}

static obs_properties_t *coreaudio_output_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	return coreaudio_properties(false);
}

struct obs_source_info coreaudio_input_capture_info = {
	.id = "coreaudio_input_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = coreaudio_input_getname,
	.create = coreaudio_create_input_capture,
	.destroy = coreaudio_destroy,
	.update = coreaudio_update,
	.get_defaults = coreaudio_defaults,
	.get_properties = coreaudio_input_properties,
	.icon_type = OBS_ICON_TYPE_AUDIO_INPUT,
};

struct obs_source_info coreaudio_output_capture_info = {
	.id = "coreaudio_output_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE |
			OBS_SOURCE_DO_NOT_SELF_MONITOR,
	.get_name = coreaudio_output_getname,
	.create = coreaudio_create_output_capture,
	.destroy = coreaudio_destroy,
	.update = coreaudio_update,
	.get_defaults = coreaudio_defaults,
	.get_properties = coreaudio_output_properties,
	.icon_type = OBS_ICON_TYPE_AUDIO_OUTPUT,
};

static void maccapture_macaudio(void *priv_data, calldata_t *data)
{
	UNUSED_PARAMETER(priv_data);
	const char *name = calldata_string(data, "name");
	bool ret_value;
	if (strcmp(name, "is_hal_device_installed") == 0) {
		ret_value = is_hal_device_installed();
	} else if (strcmp(name, "is_multioutput_installed") == 0) {
		ret_value = is_multioutput_device_installed();
	} else if (strcmp(name, "is_hal_device_updated") == 0) {
		ret_value = is_hal_device_updated();
	} else if (strcmp(name, "install_all") == 0) {
		if (is_multioutput_device_installed())
			delete_multioutput_device();
		if (create_multioutput_device()) {
			ret_value = modify_hal_plugin(true);
			if (!ret_value)
				delete_multioutput_device();
		} else {
			ret_value = false;
		}
	} else if (strcmp(name, "uninstall_all") == 0) {
		if (delete_multioutput_device()) {
			ret_value = modify_hal_plugin(false);
			if (!ret_value)
				create_multioutput_device();
		} else {
			ret_value = false;
		}
	} else if (strcmp(name, "uninstall_broken") == 0) {
		delete_multioutput_device();
		ret_value = modify_hal_plugin(false);
	} else if (strcmp(name, "update_hal_device") == 0) {
		ret_value = modify_hal_plugin(true);
	} else {
		blog(LOG_WARNING, "[mac-capture]: Unknown procedure call: '%s'",
		     name);
		return;
	}
	calldata_set_bool(data, "value", ret_value);
}

void macaudio_add_proc()
{
	proc_handler_t *global = obs_get_proc_handler();
	proc_handler_add(
		global,
		"void maccapture_macaudio(in string name, out bool value)",
		maccapture_macaudio, NULL);
}
