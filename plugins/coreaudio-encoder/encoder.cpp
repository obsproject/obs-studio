#include <util/darray.h>
#include <util/dstr.hpp>
#include <obs-module.h>

#include <initializer_list>
#include <memory>
#include <vector>

#ifndef _WIN32
#include <AudioToolbox/AudioToolbox.h>
#endif

#define CA_LOG(level, format, ...) \
	blog(level, "[CoreAudio encoder]: " format, ##__VA_ARGS__)
#define CA_LOG_ENCODER(format_name, encoder, level, format, ...) \
	blog(level, "[CoreAudio %s: '%s']: " format, \
			format_name, obs_encoder_get_name(encoder), \
			##__VA_ARGS__)
#define CA_BLOG(level, format, ...) \
	CA_LOG_ENCODER(ca->format_name, ca->encoder, level, format, \
			##__VA_ARGS__)
#define CA_CO_LOG(level, format, ...) \
	do { \
		if (ca) \
			CA_BLOG(level, format, ##__VA_ARGS__); \
		else \
			CA_LOG(level, format, ##__VA_ARGS__); \
	} while (false)

#ifdef _WIN32
#include "windows-imports.h"
#endif

using namespace std;

namespace {

struct asbd_builder {
	AudioStreamBasicDescription asbd;

	asbd_builder &sample_rate(Float64 rate)
	{
		asbd.mSampleRate = rate;
		return *this;
	}

	asbd_builder &format_id(UInt32 format)
	{
		asbd.mFormatID = format;
		return *this;
	}

	asbd_builder &format_flags(UInt32 flags)
	{
		asbd.mFormatFlags = flags;
		return *this;
	}

	asbd_builder &bytes_per_packet(UInt32 bytes)
	{
		asbd.mBytesPerPacket = bytes;
		return *this;
	}

	asbd_builder &frames_per_packet(UInt32 frames)
	{
		asbd.mFramesPerPacket = frames;
		return *this;
	}

	asbd_builder &bytes_per_frame(UInt32 bytes)
	{
		asbd.mBytesPerFrame = bytes;
		return *this;
	}

	asbd_builder &channels_per_frame(UInt32 channels)
	{
		asbd.mChannelsPerFrame = channels;
		return *this;
	}

	asbd_builder &bits_per_channel(UInt32 bits)
	{
		asbd.mBitsPerChannel = bits;
		return *this;
	}
};

struct ca_encoder {
	obs_encoder_t     *encoder;
	const char        *format_name;
	UInt32            format_id;

	const initializer_list<UInt32> *allowed_formats;

	AudioConverterRef converter;

	size_t            output_buffer_size;
	vector<uint8_t>   output_buffer;

	size_t            out_frames_per_packet;

	size_t            in_packets;
	size_t            in_frame_size;
	size_t            in_bytes_required;

	DARRAY(uint8_t)   input_buffer;
	size_t            bytes_read;

	uint64_t          total_samples;
	uint64_t          samples_per_second;

	vector<uint8_t>   extra_data;

	size_t            channels;

	~ca_encoder()
	{
		if (converter)
			AudioConverterDispose(converter);

		da_free(input_buffer);
	}
};
typedef struct ca_encoder ca_encoder;

}

#ifndef _WIN32
namespace std {

template <>
struct default_delete<remove_pointer<CFErrorRef>::type> {
	void operator()(remove_pointer<CFErrorRef>::type *err)
	{
		CFRelease(err);
	}
};

template <>
struct default_delete<remove_pointer<CFStringRef>::type> {
	void operator()(remove_pointer<CFStringRef>::type *str)
	{
		CFRelease(str);
	}
};

}

template <typename T>
using cf_ptr = unique_ptr<typename remove_pointer<T>::type>;
#endif

#ifndef _MSC_VER
__attribute__((__format__(__printf__, 3, 4)))
#endif
static void log_to_dstr(DStr &str, ca_encoder *ca, const char *fmt, ...)
{
	dstr prev_str = *static_cast<dstr*>(str);

	va_list args;
	va_start(args, fmt);
	dstr_vcatf(str, fmt, args);
	va_end(args);

	if (str->array)
		return;

	char array[4096];
	va_start(args, fmt);
	vsnprintf(array, 4096, fmt, args);
	va_end(args);

	array[4095] = 0;

	if (!prev_str.array && !prev_str.len)
		CA_CO_LOG(LOG_ERROR, "Could not allocate buffer for logging:"
				"\n'%s'", array);
	else
		CA_CO_LOG(LOG_ERROR, "Could not allocate buffer for logging:"
				"\n'%s'\nPrevious log entries:\n%s",
				array, prev_str.array);

	bfree(prev_str.array);
}

static const char *flush_log(DStr &log)
{
	if (!log->array || !log->len)
		return "";

	if (log->array[log->len - 1] == '\n') {
		log->array[log->len - 1] = 0; //Get rid of last newline
		log->len -= 1;
	}

	return log->array;
}

#define CA_CO_DLOG_(level, format) \
	CA_CO_LOG(level, format "%s%s", \
			log->array ? ":\n" : "", flush_log(log))
#define CA_CO_DLOG(level, format, ...) \
	CA_CO_LOG(level, format "%s%s", ##__VA_ARGS__, \
			log->array ? ":\n" : "", flush_log(log))

static const char *aac_get_name(void)
{
	return obs_module_text("CoreAudioAAC");
}

static const char *code_to_str(OSStatus code)
{
	switch (code) {
#define HANDLE_CODE(c) case c: return #c
	HANDLE_CODE(kAudio_UnimplementedError);
	HANDLE_CODE(kAudio_FileNotFoundError);
	HANDLE_CODE(kAudio_FilePermissionError);
	HANDLE_CODE(kAudio_TooManyFilesOpenError);
	HANDLE_CODE(kAudio_BadFilePathError);
	HANDLE_CODE(kAudio_ParamError);
	HANDLE_CODE(kAudio_MemFullError);

	HANDLE_CODE(kAudioConverterErr_FormatNotSupported);
	HANDLE_CODE(kAudioConverterErr_OperationNotSupported);
	HANDLE_CODE(kAudioConverterErr_PropertyNotSupported);
	HANDLE_CODE(kAudioConverterErr_InvalidInputSize);
	HANDLE_CODE(kAudioConverterErr_InvalidOutputSize);
	HANDLE_CODE(kAudioConverterErr_UnspecifiedError);
	HANDLE_CODE(kAudioConverterErr_BadPropertySizeError);
	HANDLE_CODE(kAudioConverterErr_RequiresPacketDescriptionsError);
	HANDLE_CODE(kAudioConverterErr_InputSampleRateOutOfRange);
	HANDLE_CODE(kAudioConverterErr_OutputSampleRateOutOfRange);
#undef HANDLE_CODE

	default: break;
	}

	return NULL;
}

static DStr osstatus_to_dstr(OSStatus code)
{
	DStr result;

#ifndef _WIN32
	cf_ptr<CFErrorRef> err{CFErrorCreate(kCFAllocatorDefault,
			kCFErrorDomainOSStatus, code, NULL)};
	cf_ptr<CFStringRef> str{CFErrorCopyDescription(err.get())};

	CFIndex length   = CFStringGetLength(str.get());
	CFIndex max_size = CFStringGetMaximumSizeForEncoding(length,
			kCFStringEncodingUTF8);

	dstr_ensure_capacity(result, max_size);

	if (result->array && CFStringGetCString(str.get(), result->array,
				max_size, kCFStringEncodingUTF8)) {
		dstr_resize(result, strlen(result->array));
		return result;
	}
#endif

	const char *code_str = code_to_str(code);
	dstr_printf(result, "%s%s%d%s",
			code_str ? code_str : "",
			code_str ? " (" : "",
			static_cast<int>(code),
			code_str ? ")" : "");
	return result;
}

static void log_osstatus(int log_level, ca_encoder *ca, const char *context,
		OSStatus code)
{
	DStr str = osstatus_to_dstr(code);
	if (ca)
		CA_BLOG(log_level, "Error in %s: %s", context, str->array);
	else
		CA_LOG(log_level, "Error in %s: %s", context, str->array);
}

static const char *format_id_to_str(UInt32 format_id)
{
#define FORMAT_TO_STR(x) case x: return #x
	switch (format_id) {
	FORMAT_TO_STR(kAudioFormatLinearPCM);
	FORMAT_TO_STR(kAudioFormatAC3);
	FORMAT_TO_STR(kAudioFormat60958AC3);
	FORMAT_TO_STR(kAudioFormatAppleIMA4);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC);
	FORMAT_TO_STR(kAudioFormatMPEG4CELP);
	FORMAT_TO_STR(kAudioFormatMPEG4HVXC);
	FORMAT_TO_STR(kAudioFormatMPEG4TwinVQ);
	FORMAT_TO_STR(kAudioFormatMACE3);
	FORMAT_TO_STR(kAudioFormatMACE6);
	FORMAT_TO_STR(kAudioFormatULaw);
	FORMAT_TO_STR(kAudioFormatALaw);
	FORMAT_TO_STR(kAudioFormatQDesign);
	FORMAT_TO_STR(kAudioFormatQDesign2);
	FORMAT_TO_STR(kAudioFormatQUALCOMM);
	FORMAT_TO_STR(kAudioFormatMPEGLayer1);
	FORMAT_TO_STR(kAudioFormatMPEGLayer2);
	FORMAT_TO_STR(kAudioFormatMPEGLayer3);
	FORMAT_TO_STR(kAudioFormatTimeCode);
	FORMAT_TO_STR(kAudioFormatMIDIStream);
	FORMAT_TO_STR(kAudioFormatParameterValueStream);
	FORMAT_TO_STR(kAudioFormatAppleLossless);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC_HE);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC_LD);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC_ELD);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC_ELD_SBR);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC_HE_V2);
	FORMAT_TO_STR(kAudioFormatMPEG4AAC_Spatial);
	FORMAT_TO_STR(kAudioFormatAMR);
	FORMAT_TO_STR(kAudioFormatAudible);
	FORMAT_TO_STR(kAudioFormatiLBC);
	FORMAT_TO_STR(kAudioFormatDVIIntelIMA);
	FORMAT_TO_STR(kAudioFormatMicrosoftGSM);
	FORMAT_TO_STR(kAudioFormatAES3);
	}
#undef FORMAT_TO_STR

	return "Unknown format";
}

static void aac_destroy(void *data)
{
	ca_encoder *ca = static_cast<ca_encoder*>(data);

	delete ca;
}

template <typename Func>
static bool query_converter_property_raw(DStr &log, ca_encoder *ca,
		AudioFormatPropertyID property,
		const char *get_property_info, const char *get_property,
		AudioConverterRef converter, Func &&func)
{
	UInt32 size = 0;
	OSStatus code = AudioConverterGetPropertyInfo(converter, property,
			&size, nullptr);
	if (code) {
		log_to_dstr(log, ca, "%s: %s\n", get_property_info,
				osstatus_to_dstr(code)->array);
		return false;
	}

	if (!size) {
		log_to_dstr(log, ca, "%s returned 0 size\n", get_property_info);
		return false;
	}

	vector<uint8_t> buffer;
	
	try {
		buffer.resize(size);
	} catch (...) {
		log_to_dstr(log, ca, "Failed to allocate %u bytes for %s\n",
				static_cast<uint32_t>(size), get_property);
		return false;
	}

	code = AudioConverterGetProperty(converter, property, &size,
			buffer.data());
	if (code) {
		log_to_dstr(log, ca, "%s: %s\n", get_property,
				osstatus_to_dstr(code)->array);
		return false;
	}

	func(size, static_cast<void*>(buffer.data()));

	return true;
}

#define EXPAND_CONVERTER_NAMES(x) x, \
	"AudioConverterGetPropertyInfo(" #x ")", \
	"AudioConverterGetProperty(" #x ")"

template <typename Func>
static bool enumerate_bitrates(DStr &log, ca_encoder *ca,
		AudioConverterRef converter, Func &&func)
{
	auto helper = [&](UInt32 size, void *data)
	{
		auto range = static_cast<AudioValueRange*>(data);
		size_t num_ranges = size / sizeof(AudioValueRange);
		for (size_t i = 0; i < num_ranges; i++)
			func(static_cast<UInt32>(range[i].mMinimum),
					static_cast<UInt32>(range[i].mMaximum));
	};

	return query_converter_property_raw(log, ca, EXPAND_CONVERTER_NAMES(
			kAudioConverterApplicableEncodeBitRates),
			converter, helper);
}

typedef void (*bitrate_enumeration_func)(void *data, UInt32 min, UInt32 max);

static bool enumerate_bitrates(ca_encoder *ca, AudioConverterRef converter,
		bitrate_enumeration_func enum_func, void *data)
{
	if (!converter && ca)
		converter = ca->converter;

	UInt32 size;
	OSStatus code = AudioConverterGetPropertyInfo(converter,
			kAudioConverterApplicableEncodeBitRates,
			&size, NULL);
	if (code) {
		log_osstatus(LOG_WARNING, ca,
				"AudioConverterGetPropertyInfo(bitrates)",
				code);
		return false;
	}

	if (!size) {
		if (ca)
			CA_BLOG(LOG_WARNING, "Query for applicable bitrates "
					"returned 0 size");
		else
			CA_LOG(LOG_WARNING, "Query for applicable bitrates "
					"returned 0 size");
		return false;
	}

	size_t num_bitrates = (size + sizeof(AudioValueRange) - 1) /
		sizeof(AudioValueRange);
	vector<AudioValueRange> bitrates;
	
	try {
		bitrates.resize(num_bitrates);
	} catch (...) {
		if (ca)
			CA_BLOG(LOG_WARNING, "Could not allocate buffer while "
					"enumerating bitrates");
		else
			CA_LOG(LOG_WARNING, "Could not allocate buffer while "
					"enumerating bitrates");
		return false;
	}

	code = AudioConverterGetProperty(converter,
			kAudioConverterApplicableEncodeBitRates,
			&size, bitrates.data());
	if (code) {
		log_osstatus(LOG_WARNING, ca,
				"AudioConverterGetProperty(bitrates)", code);

		return false;
	}

	for (size_t i = 0; i < num_bitrates; i++)
		enum_func(data, (UInt32)bitrates[i].mMinimum,
				(UInt32)bitrates[i].mMaximum);

	return num_bitrates > 0;
}

static bool bitrate_valid(DStr &log, ca_encoder *ca,
		AudioConverterRef converter, UInt32 bitrate)
{
	bool valid = false;

	auto helper = [&](UInt32 min_, UInt32 max_)
	{
		if (min_ == bitrate || max_ == bitrate)
			valid = true;
	};

	enumerate_bitrates(log, ca, converter, helper);

	return valid;
}

static bool create_encoder(DStr &log, ca_encoder *ca,
		AudioStreamBasicDescription *in,
		AudioStreamBasicDescription *out,
		UInt32 format_id, UInt32 bitrate, UInt32 rate_control)
{
#define STATUS_CHECK(c) \
	code = c; \
	if (code) { \
		log_to_dstr(log, ca, #c " returned %s", \
				osstatus_to_dstr(code)->array); \
		return false; \
	}

	auto out_ = asbd_builder()
		.sample_rate((Float64)ca->samples_per_second)
		.channels_per_frame((UInt32)ca->channels)
		.format_id(format_id)
		.asbd;

	UInt32 size = sizeof(*out);
	OSStatus code;
	STATUS_CHECK(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo,
			0, NULL, &size, &out_));

	*out = out_;

	STATUS_CHECK(AudioConverterNew(in, out, &ca->converter))

	STATUS_CHECK(AudioConverterSetProperty(ca->converter,
			kAudioCodecPropertyBitRateControlMode,
			sizeof(rate_control), &rate_control));

	if (!bitrate_valid(log, ca, ca->converter, bitrate)) {
		log_to_dstr(log, ca, "Encoder does not support bitrate %u "
				"for format %s (0x%x)\n",
				(uint32_t)bitrate, format_id_to_str(format_id),
				(uint32_t)format_id);
		return false;
	}

	ca->format_id = format_id;

	return true;
#undef STATUS_CHECK
}

static const initializer_list<UInt32> aac_formats = {
	kAudioFormatMPEG4AAC_HE_V2,
	kAudioFormatMPEG4AAC_HE,
	kAudioFormatMPEG4AAC,
};

static const initializer_list<UInt32> aac_lc_formats = {
	kAudioFormatMPEG4AAC,
};

static void *aac_create(obs_data_t *settings, obs_encoder_t *encoder)
{
#define STATUS_CHECK(c) \
	code = c; \
	if (code) { \
		log_osstatus(LOG_ERROR, ca.get(), #c, code); \
		return nullptr; \
	}

	UInt32 bitrate = (UInt32)obs_data_get_int(settings, "bitrate") * 1000;
	if (!bitrate) {
		CA_LOG_ENCODER("AAC", encoder, LOG_ERROR,
				"Invalid bitrate specified");
		return NULL;
	}

	const enum audio_format format = AUDIO_FORMAT_FLOAT;

	if (is_audio_planar(format)) {
		CA_LOG_ENCODER("AAC", encoder, LOG_ERROR,
				"Got non-interleaved audio format %d", format);
		return NULL;
	}

	unique_ptr<ca_encoder> ca;

	try {
		ca.reset(new ca_encoder());
	} catch (...) {
		CA_LOG_ENCODER("AAC", encoder, LOG_ERROR,
				"Could not allocate encoder");
		return nullptr;
	}

	ca->encoder = encoder;
	ca->format_name = "AAC";

	audio_t *audio = obs_encoder_audio(encoder);
	const struct audio_output_info *aoi = audio_output_get_info(audio);

	ca->channels = audio_output_get_channels(audio);
	ca->samples_per_second = audio_output_get_sample_rate(audio);

	size_t bytes_per_frame  = get_audio_size(format, aoi->speakers, 1);
	size_t bits_per_channel = get_audio_bytes_per_channel(format) * 8;

	auto in = asbd_builder()
		.sample_rate((Float64)ca->samples_per_second)
		.channels_per_frame((UInt32)ca->channels)
		.bytes_per_frame((UInt32)bytes_per_frame)
		.frames_per_packet(1)
		.bytes_per_packet((UInt32)(1 * bytes_per_frame))
		.bits_per_channel((UInt32)bits_per_channel)
		.format_id(kAudioFormatLinearPCM)
		.format_flags(kAudioFormatFlagsNativeEndian |
			kAudioFormatFlagIsPacked |
			kAudioFormatFlagIsFloat |
			0)
		.asbd;

	AudioStreamBasicDescription out;

	UInt32 rate_control = kAudioCodecBitRateControlMode_Constant;

	if (obs_data_get_bool(settings, "allow he-aac")) {
		ca->allowed_formats = &aac_formats;
	} else {
		ca->allowed_formats = &aac_lc_formats;
	}

	DStr log;

	bool encoder_created = false;
	for (UInt32 format_id : *ca->allowed_formats) {
		log_to_dstr(log, ca.get(), "Trying format %s (0x%x)\n",
				format_id_to_str(format_id),
				(uint32_t)format_id);

		if (!create_encoder(log, ca.get(), &in, &out, format_id,
					bitrate, rate_control))
			continue;

		encoder_created = true;
		break;
	}

	if (!encoder_created) {
		CA_CO_DLOG(LOG_ERROR, "Could not create encoder for "
				"selected format%s",
				ca->allowed_formats->size() == 1 ? "" : "s");
		return nullptr;
	}

	if (log->len)
		CA_CO_DLOG_(LOG_DEBUG, "Encoder created");

	OSStatus code;
	UInt32 converter_quality = kAudioConverterQuality_Max;
	STATUS_CHECK(AudioConverterSetProperty(ca->converter,
			kAudioConverterCodecQuality,
			sizeof(converter_quality), &converter_quality));

	STATUS_CHECK(AudioConverterSetProperty(ca->converter,
			kAudioConverterEncodeBitRate,
			sizeof(bitrate), &bitrate));

	UInt32 size = sizeof(in);
	STATUS_CHECK(AudioConverterGetProperty(ca->converter,
			kAudioConverterCurrentInputStreamDescription,
			&size, &in));

	size = sizeof(out);
	STATUS_CHECK(AudioConverterGetProperty(ca->converter,
			kAudioConverterCurrentOutputStreamDescription,
			&size, &out));

	ca->in_frame_size     = in.mBytesPerFrame;
	ca->in_packets        = out.mFramesPerPacket / in.mFramesPerPacket;
	ca->in_bytes_required = ca->in_packets * ca->in_frame_size;

	ca->out_frames_per_packet = out.mFramesPerPacket;

	da_init(ca->input_buffer);

	ca->output_buffer_size = out.mBytesPerPacket;

	if (out.mBytesPerPacket == 0) {
		UInt32 max_packet_size = 0;
		size = sizeof(max_packet_size);
		
		code = AudioConverterGetProperty(ca->converter,
				kAudioConverterPropertyMaximumOutputPacketSize,
				&size, &max_packet_size);
		if (code) {
			log_osstatus(LOG_WARNING, ca.get(),
					"AudioConverterGetProperty(PacketSz)",
					code);
			ca->output_buffer_size = 32768;
		} else {
			ca->output_buffer_size = max_packet_size;
		}
	}

	try {
		ca->output_buffer.resize(ca->output_buffer_size);
	} catch (...) {
		CA_BLOG(LOG_ERROR, "Failed to allocate output buffer");
		return nullptr;
	}

	const char *format_name =
		out.mFormatID == kAudioFormatMPEG4AAC_HE_V2 ? "HE-AAC v2" :
		out.mFormatID == kAudioFormatMPEG4AAC_HE    ? "HE-AAC" : "AAC";
	CA_BLOG(LOG_INFO, "settings:\n"
			"\tmode:          %s\n"
			"\tbitrate:       %u\n"
			"\tsample rate:   %llu\n"
			"\tcbr:           %s\n"
			"\toutput buffer: %lu",
			format_name, (unsigned int)bitrate / 1000,
			ca->samples_per_second,
			rate_control == kAudioCodecBitRateControlMode_Constant ?
			"on" : "off",
			(unsigned long)ca->output_buffer_size);

	return ca.release();
}

static OSStatus complex_input_data_proc(AudioConverterRef inAudioConverter,
		UInt32 *ioNumberDataPackets, AudioBufferList *ioData,
		AudioStreamPacketDescription **outDataPacketDescription,
		void *inUserData)
{
	UNUSED_PARAMETER(inAudioConverter);
	UNUSED_PARAMETER(outDataPacketDescription);

	ca_encoder *ca = static_cast<ca_encoder*>(inUserData);

	if (ca->bytes_read) {
		da_erase_range(ca->input_buffer, 0, ca->bytes_read);
		ca->bytes_read = 0;
	}

	if (ca->input_buffer.num < ca->in_bytes_required) {
		*ioNumberDataPackets = 0;
		ioData->mBuffers[0].mData = NULL;
		return 1;
	}

	*ioNumberDataPackets =
		(UInt32)(ca->in_bytes_required / ca->in_frame_size);
	ioData->mNumberBuffers = 1;

	ioData->mBuffers[0].mData = ca->input_buffer.array;
	ioData->mBuffers[0].mNumberChannels = (UInt32)ca->channels;
	ioData->mBuffers[0].mDataByteSize = (UInt32)ca->in_bytes_required;

	ca->bytes_read += ca->in_packets * ca->in_frame_size;

	return 0;
}

#ifdef _MSC_VER
// disable warning that recommends if ((foo = bar > 0) == false) over
// if (!(foo = bar > 0))
#pragma warning(push)
#pragma warning(disable: 4706)
#endif
static bool aac_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	ca_encoder *ca = static_cast<ca_encoder*>(data);

	da_push_back_array(ca->input_buffer, frame->data[0],
			frame->linesize[0]);

	if ((ca->input_buffer.num - ca->bytes_read) < ca->in_bytes_required)
		return true;

	UInt32 packets = 1;

	AudioBufferList buffer_list = { 0 };
	buffer_list.mNumberBuffers = 1;
	buffer_list.mBuffers[0].mNumberChannels = (UInt32)ca->channels;
	buffer_list.mBuffers[0].mDataByteSize = (UInt32)ca->output_buffer_size;
	buffer_list.mBuffers[0].mData = ca->output_buffer.data();

	AudioStreamPacketDescription out_desc = { 0 };

	OSStatus code = AudioConverterFillComplexBuffer(ca->converter,
			complex_input_data_proc, ca, &packets,
			&buffer_list, &out_desc);
	if (code && code != 1) {
		log_osstatus(LOG_ERROR, ca, "AudioConverterFillComplexBuffer",
				code);
		return false;
	}

	if (!(*received_packet = packets > 0))
		return true;

	packet->pts = ca->total_samples;
	packet->dts = ca->total_samples;
	packet->timebase_num = 1;
	packet->timebase_den = (uint32_t)ca->samples_per_second;
	packet->type = OBS_ENCODER_AUDIO;
	packet->size = out_desc.mDataByteSize;
	packet->data =
		(uint8_t*)buffer_list.mBuffers[0].mData + out_desc.mStartOffset;

	ca->total_samples += ca->bytes_read / ca->in_frame_size;

	return true;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

static void aac_audio_info(void *data, struct audio_convert_info *info)
{
	UNUSED_PARAMETER(data);

	info->format = AUDIO_FORMAT_FLOAT;
}

static size_t aac_frame_size(void *data)
{
	ca_encoder *ca = static_cast<ca_encoder*>(data);
	return ca->out_frames_per_packet;
}

/* The following code was extracted from encca_aac.c in HandBrake's libhb */
#define MP4ESDescrTag                   0x03
#define MP4DecConfigDescrTag            0x04
#define MP4DecSpecificDescrTag          0x05

// based off of mov_mp4_read_descr_len from mov.c in ffmpeg's libavformat
static int read_descr_len(uint8_t **buffer)
{
	int len = 0;
	int count = 4;
	while (count--)
	{
		int c = *(*buffer)++;
		len = (len << 7) | (c & 0x7f);
		if (!(c & 0x80))
			break;
	}
	return len;
}

// based off of mov_mp4_read_descr from mov.c in ffmpeg's libavformat
static int read_descr(uint8_t **buffer, int *tag)
{
	*tag = *(*buffer)++;
	return read_descr_len(buffer);
}

// based off of mov_read_esds from mov.c in ffmpeg's libavformat
static void read_esds_desc_ext(uint8_t* desc_ext, vector<uint8_t> &buffer,
		bool version_flags)
{
	uint8_t *esds = desc_ext;
	int tag, len;

	if (version_flags)
		esds += 4; // version + flags

	read_descr(&esds, &tag);
	esds += 2;     // ID
	if (tag == MP4ESDescrTag)
		esds++;    // priority

	read_descr(&esds, &tag);
	if (tag == MP4DecConfigDescrTag) {
		esds++;    // object type id
		esds++;    // stream type
		esds += 3; // buffer size db
		esds += 4; // max bitrate
		esds += 4; // average bitrate

		len = read_descr(&esds, &tag);
		if (tag == MP4DecSpecificDescrTag)
			try {
				buffer.assign(esds, esds + len);
			} catch (...) {
				//leave buffer empty
			}
	}
}
/* extracted code ends here */

static void query_extra_data(ca_encoder *ca)
{
	UInt32 size = 0;

	OSStatus code;
	code = AudioConverterGetPropertyInfo(ca->converter,
			kAudioConverterCompressionMagicCookie,
			&size, NULL);
	if (code) {
		log_osstatus(LOG_ERROR, ca,
				"AudioConverterGetPropertyInfo(magic_cookie)",
				code);
		return;
	}

	if (!size) {
		CA_BLOG(LOG_WARNING, "Got 0 data size info for magic_cookie");
		return;
	}

	vector<uint8_t> extra_data;
	
	try {
		extra_data.resize(size);
	} catch (...) {
		CA_BLOG(LOG_WARNING, "Could not allocate extra data buffer");
		return;
	}

	code = AudioConverterGetProperty(ca->converter,
			kAudioConverterCompressionMagicCookie,
			&size, extra_data.data());
	if (code) {
		log_osstatus(LOG_ERROR, ca,
				"AudioConverterGetProperty(magic_cookie)",
				code);
		return;
	}

	if (!size) {
		CA_BLOG(LOG_WARNING, "Got 0 data size for magic_cookie");
		return;
	}

	read_esds_desc_ext(extra_data.data(), ca->extra_data, false);
}

static bool aac_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	ca_encoder *ca = static_cast<ca_encoder*>(data);

	if (!ca->extra_data.size())
		query_extra_data(ca);

	if (!ca->extra_data.size())
		return false;

	*extra_data = ca->extra_data.data();
	*size = ca->extra_data.size();
	return true;
}

static AudioConverterRef get_default_converter(UInt32 format_id)
{
	UInt32 bytes_per_frame = 8;
	UInt32 channels = 2;
	UInt32 bits_per_channel = bytes_per_frame / channels * 8;

	auto in = asbd_builder()
		.sample_rate(44100)
		.channels_per_frame(channels)
		.bytes_per_frame(bytes_per_frame)
		.frames_per_packet(1)
		.bytes_per_packet(1 * bytes_per_frame)
		.bits_per_channel(bits_per_channel)
		.format_id(kAudioFormatLinearPCM)
		.format_flags(kAudioFormatFlagsNativeEndian |
			kAudioFormatFlagIsPacked |
			kAudioFormatFlagIsFloat |
			0)
		.asbd;

	auto out = asbd_builder()
		.sample_rate(44100)
		.channels_per_frame(channels)
		.format_id(format_id)
		.asbd;

	UInt32 size = sizeof(out);
	OSStatus code = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo,
			0, NULL, &size, &out);
	if (code) {
		log_osstatus(LOG_WARNING, NULL,
				"AudioFormatGetProperty(format_info)", code);
		return NULL;
	}

	AudioConverterRef converter;
	code = AudioConverterNew(&in, &out, &converter);
	if (code) {
		log_osstatus(LOG_WARNING, NULL, "AudioConverterNew", code);
		return NULL;
	}

	return converter;
}

static AudioConverterRef aac_default_converter(void)
{
	return get_default_converter(kAudioFormatMPEG4AAC);
}

static AudioConverterRef he_aac_default_converter(void)
{
	return get_default_converter(kAudioFormatMPEG4AAC_HE);
}

struct find_matching_bitrate_helper {
	UInt32 bitrate;
	UInt32 best_match;
	int diff;
};
typedef struct find_matching_bitrate_helper find_matching_bitrate_helper;

static void find_matching_bitrate_func(void *data, UInt32 min, UInt32 max)
{
	find_matching_bitrate_helper *helper =
		static_cast<find_matching_bitrate_helper*>(data);

	int min_diff = abs((int)helper->bitrate - (int)min);
	int max_diff = abs((int)helper->bitrate - (int)max);

	if (min_diff < helper->diff) {
		helper->best_match = min;
		helper->diff = min_diff;
	}

	if (max_diff < helper->diff) {
		helper->best_match = max;
		helper->diff = max_diff;
	}
}

static UInt32 find_matching_bitrate(UInt32 bitrate)
{
	find_matching_bitrate_helper helper;
	helper.bitrate = bitrate * 1000;
	helper.best_match = 0;
	helper.diff = INT_MAX;

	AudioConverterRef converter = aac_default_converter();
	if (!converter) {
		CA_LOG(LOG_ERROR, "Could not get converter to match "
				"default bitrate");
		return bitrate;
	}

	bool has_bitrates = enumerate_bitrates(NULL, converter,
			find_matching_bitrate_func, &helper);
	AudioConverterDispose(converter);

	if (!has_bitrates) {
		CA_LOG(LOG_ERROR, "No bitrates found while matching "
				"default bitrate");
		AudioConverterDispose(converter);
		return bitrate;
	}

	if (helper.best_match != helper.bitrate)
		CA_LOG(LOG_INFO, "Returning closest matching bitrate %u "
				"instead of requested bitrate %u",
				(uint32_t)helper.best_match / 1000,
				(uint32_t)bitrate);

	return helper.best_match / 1000;
}

static void aac_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate",
			find_matching_bitrate(128));
	obs_data_set_default_bool(settings, "allow he-aac", true);
}

struct add_bitrates_helper {
	DARRAY(UInt32) bitrates;
};
typedef struct add_bitrates_helper add_bitrates_helper;

static void add_bitrates_func(void *data, UInt32 min, UInt32 max)
{
	add_bitrates_helper *helper =
		static_cast<add_bitrates_helper*>(data);

	if (da_find(helper->bitrates, &min, 0) == DARRAY_INVALID)
		da_push_back(helper->bitrates, &min);
	if (da_find(helper->bitrates, &max, 0) == DARRAY_INVALID)
		da_push_back(helper->bitrates, &max);
}

static int bitrate_compare(const void *data1, const void *data2)
{
	const UInt32 *bitrate1 = static_cast<const UInt32*>(data1);
	const UInt32 *bitrate2 = static_cast<const UInt32*>(data2);

	return (int)*bitrate1 - (int)*bitrate2;
}

static void add_bitrates(obs_property_t *prop, ca_encoder *ca)
{
	add_bitrates_helper helper = { { {0} } };

	for (UInt32 format_id : (ca ? *ca->allowed_formats : aac_formats))
		enumerate_bitrates(ca,
				get_default_converter(format_id),
				add_bitrates_func, &helper);

	if (!helper.bitrates.num) {
		CA_BLOG(LOG_ERROR, "Enumeration found no available bitrates");
		return;
	}

	qsort(helper.bitrates.array, helper.bitrates.num, sizeof(UInt32),
			bitrate_compare);

	struct dstr str = { 0 };
	for (size_t i = 0; i < helper.bitrates.num; i++) {
		dstr_printf(&str, "%u",
				(uint32_t)helper.bitrates.array[i]/1000);
		obs_property_list_add_int(prop, str.array,
				helper.bitrates.array[i]/1000);
	}
	dstr_free(&str);
	da_free(helper.bitrates);
}

static obs_properties_t *aac_properties(void *data)
{
	ca_encoder *ca = static_cast<ca_encoder*>(data);

	obs_properties_t *props = obs_properties_create();

	obs_property_t *p = obs_properties_add_list(props, "bitrate",
			obs_module_text("Bitrate"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	add_bitrates(p, ca);

	obs_properties_add_bool(props, "allow he-aac",
			obs_module_text("AllowHEAAC"));

	return props;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("coreaudio-encoder", "en-US")

bool obs_module_load(void)
{
#ifdef _WIN32
	if (!load_core_audio()) {
		CA_LOG(LOG_WARNING, "Couldn't load CoreAudio AAC encoder");
		return true;
	}

	CA_LOG(LOG_INFO, "Adding CoreAudio AAC encoder");
#endif

	struct obs_encoder_info aac_info;
	aac_info.id = "CoreAudio_AAC";
	aac_info.type = OBS_ENCODER_AUDIO;
	aac_info.codec = "AAC";
	aac_info.get_name = aac_get_name;
	aac_info.destroy = aac_destroy;
	aac_info.create = aac_create;
	aac_info.encode = aac_encode;
	aac_info.get_frame_size = aac_frame_size;
	aac_info.get_audio_info = aac_audio_info;
	aac_info.get_extra_data = aac_extra_data;
	aac_info.get_defaults = aac_defaults;
	aac_info.get_properties = aac_properties;

	obs_register_encoder(&aac_info);
	return true;
}

#ifdef _WIN32
void obs_module_unload(void)
{
	unload_core_audio();
}
#endif
