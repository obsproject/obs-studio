#include <util/darray.h>
#include <util/dstr.h>
#include <obs-module.h>

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

#ifdef _WIN32
#include "windows-imports.h"
#endif

struct ca_encoder {
	obs_encoder_t     *encoder;
	const char        *format_name;
	UInt32            format_id;

	const UInt32      *allowed_formats;
	size_t            num_allowed_formats;

	AudioConverterRef converter;

	size_t            output_buffer_size;
	uint8_t           *output_buffer;

	size_t            out_frames_per_packet;

	size_t            in_packets;
	size_t            in_frame_size;
	size_t            in_bytes_required;

	DARRAY(uint8_t)   input_buffer;
	size_t            bytes_read;

	uint64_t          total_samples;
	uint64_t          samples_per_second;

	uint8_t           *extra_data;
	uint32_t          extra_data_size;

	size_t            channels;
};
typedef struct ca_encoder ca_encoder;


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

static void log_osstatus(int log_level, ca_encoder *ca, const char *context,
		OSStatus code)
{
#ifndef _WIN32
	CFErrorRef err  = CFErrorCreate(kCFAllocatorDefault,
			kCFErrorDomainOSStatus, code, NULL);
	CFStringRef str = CFErrorCopyDescription(err);

	CFIndex length   = CFStringGetLength(str);
	CFIndex max_size = CFStringGetMaximumSizeForEncoding(length,
			kCFStringEncodingUTF8);

	char *c_str = malloc(max_size);
	if (CFStringGetCString(str, c_str, max_size, kCFStringEncodingUTF8)) {
		if (ca)
			CA_BLOG(log_level, "Error in %s: %s", context, c_str);
		else
			CA_LOG(log_level, "Error in %s: %s", context, c_str);
	} else {
#endif
		const char *code_str = code_to_str(code);
		if (ca)
			CA_BLOG(log_level, "Error in %s: %s%s%d%s", context,
					code_str ? code_str : "",
					code_str ? " (" : "",
					(int)code,
					code_str ? ")" : "");
		else
			CA_LOG(log_level, "Error in %s: %s%s%d%s", context,
					code_str ? code_str : "",
					code_str ? " (" : "",
					(int)code,
					code_str ? ")" : "");
#ifndef _WIN32
	}
	free(c_str);

	CFRelease(str);
	CFRelease(err);
#endif
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
	ca_encoder *ca = data;

	if (ca->converter)
		AudioConverterDispose(ca->converter);

	da_free(ca->input_buffer);
	bfree(ca->extra_data);
	bfree(ca->output_buffer);
	bfree(ca);
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

	AudioValueRange *bitrates = malloc(size);

	code = AudioConverterGetProperty(converter,
			kAudioConverterApplicableEncodeBitRates,
			&size, bitrates);
	if (code) {
		log_osstatus(LOG_WARNING, ca,
				"AudioConverterGetProperty(bitrates)", code);
		return false;
	}

	size_t num_bitrates = size / sizeof(AudioValueRange);
	for (size_t i = 0; i < num_bitrates; i++)
		enum_func(data, (UInt32)bitrates[i].mMinimum,
				(UInt32)bitrates[i].mMaximum);

	free(bitrates);

	return num_bitrates > 0;
}

struct validate_bitrate_helper {
	UInt32 bitrate;
	bool valid;
};
typedef struct validate_bitrate_helper validate_bitrate_helper;

static void validate_bitrate_func(void *data, UInt32 min, UInt32 max)
{
	validate_bitrate_helper *valid = data;

	if (valid->bitrate >= min && valid->bitrate <= max)
		valid->valid = true;
}

static bool bitrate_valid(ca_encoder *ca, AudioConverterRef converter,
		UInt32 bitrate)
{
	validate_bitrate_helper helper = {
		.bitrate = bitrate,
		.valid = false,
	};

	enumerate_bitrates(ca, converter, validate_bitrate_func, &helper);

	return helper.valid;
}

static bool create_encoder(ca_encoder *ca, AudioStreamBasicDescription *in,
		AudioStreamBasicDescription *out,
		UInt32 format_id, UInt32 bitrate, UInt32 rate_control)
{
#define STATUS_CHECK(c) \
	code = c; \
	if (code) { \
		log_osstatus(LOG_WARNING, ca, #c, code); \
		return false; \
	}

	AudioStreamBasicDescription out_ = {
		.mSampleRate = (Float64)ca->samples_per_second,
		.mChannelsPerFrame = (UInt32)ca->channels,
		.mBytesPerFrame = 0,
		.mFramesPerPacket = 0,
		.mBitsPerChannel = 0,
		.mFormatID = format_id,
		.mFormatFlags = 0
	};

	UInt32 size = sizeof(*out);
	OSStatus code;
	STATUS_CHECK(AudioFormatGetProperty(kAudioFormatProperty_FormatInfo,
			0, NULL, &size, &out_));

	*out = out_;

	STATUS_CHECK(AudioConverterNew(in, out, &ca->converter))

	STATUS_CHECK(AudioConverterSetProperty(ca->converter,
			kAudioCodecPropertyBitRateControlMode,
			sizeof(rate_control), &rate_control));

	if (!bitrate_valid(ca, NULL, bitrate)) {
		CA_BLOG(LOG_WARNING, "Encoder does not support bitrate %u for "
				"format %s (0x%x)",
				(uint32_t)bitrate, format_id_to_str(format_id),
				(uint32_t)format_id);
		return false;
	}

	ca->format_id = format_id;

	return true;
#undef STATUS_CHECK
}

static const UInt32 aac_formats[] = {
	kAudioFormatMPEG4AAC_HE_V2,
	kAudioFormatMPEG4AAC_HE,
	kAudioFormatMPEG4AAC,
};

static const UInt32 aac_lc_formats[] = {
	kAudioFormatMPEG4AAC,
};

static void *aac_create(obs_data_t *settings, obs_encoder_t *encoder)
{
#define STATUS_CHECK(c) \
	code = c; \
	if (code) { \
		log_osstatus(LOG_ERROR, ca, #c, code); \
		goto free; \
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

	ca_encoder *ca = bzalloc(sizeof(ca_encoder));
	ca->encoder = encoder;
	ca->format_name = "AAC";

	audio_t *audio = obs_encoder_audio(encoder);
	const struct audio_output_info *aoi = audio_output_get_info(audio);

	ca->channels = audio_output_get_channels(audio);
	ca->samples_per_second = audio_output_get_sample_rate(audio);

	size_t bytes_per_frame  = get_audio_size(format, aoi->speakers, 1);
	size_t bits_per_channel = get_audio_bytes_per_channel(format) * 8;

	AudioStreamBasicDescription in = {
		.mSampleRate = (Float64)ca->samples_per_second,
		.mChannelsPerFrame = (UInt32)ca->channels,
		.mBytesPerFrame = (UInt32)bytes_per_frame,
		.mFramesPerPacket = 1,
		.mBytesPerPacket = (UInt32)(1 * bytes_per_frame),
		.mBitsPerChannel = (UInt32)bits_per_channel,
		.mFormatID = kAudioFormatLinearPCM,
		.mFormatFlags = kAudioFormatFlagsNativeEndian |
			kAudioFormatFlagIsPacked |
			kAudioFormatFlagIsFloat |
			0
	};

	AudioStreamBasicDescription out;

	UInt32 rate_control = kAudioCodecBitRateControlMode_Constant;

#define USE_FORMATS(x) { \
		ca->allowed_formats = x; \
		ca->num_allowed_formats = sizeof(x)/sizeof(x[0]); \
	}

	if (obs_data_get_bool(settings, "allow he-aac")) {
		USE_FORMATS(aac_formats);
	} else {
		USE_FORMATS(aac_lc_formats);
	}

#undef USE_FORMATS

	bool encoder_created = false;
	for (size_t i = 0; i < ca->num_allowed_formats; i++) {
		UInt32 format_id = ca->allowed_formats[i];
		CA_BLOG(LOG_INFO, "Trying format %s (0x%x)",
				format_id_to_str(format_id),
				(uint32_t)format_id);

		if (!create_encoder(ca, &in, &out, format_id, bitrate,
					rate_control))
			continue;

		encoder_created = true;
		break;
	}

	if (!encoder_created) {
		CA_BLOG(LOG_ERROR, "Could not create encoder for "
				"selected format%s",
				ca->num_allowed_formats == 1 ? "" : "s");
		goto free;
	}

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
			log_osstatus(LOG_WARNING, ca,
					"AudioConverterGetProperty(PacketSz)",
					code);
			ca->output_buffer_size = 32768;
		} else {
			ca->output_buffer_size = max_packet_size;
		}
	}

	ca->output_buffer = bmalloc(ca->output_buffer_size);

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

	return ca;

free:
	aac_destroy(ca);
	return NULL;
}

static OSStatus complex_input_data_proc(AudioConverterRef inAudioConverter,
		UInt32 *ioNumberDataPackets, AudioBufferList *ioData,
		AudioStreamPacketDescription **outDataPacketDescription,
		void *inUserData)
{
	UNUSED_PARAMETER(inAudioConverter);
	UNUSED_PARAMETER(outDataPacketDescription);

	ca_encoder *ca = inUserData;

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

static bool aac_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	ca_encoder *ca = data;

	da_push_back_array(ca->input_buffer, frame->data[0],
			frame->linesize[0]);

	if ((ca->input_buffer.num - ca->bytes_read) < ca->in_bytes_required)
		return true;

	UInt32 packets = 1;

	AudioBufferList buffer_list = {
		.mNumberBuffers = 1,
		.mBuffers = { {
			.mNumberChannels = (UInt32)ca->channels,
			.mDataByteSize = (UInt32)ca->output_buffer_size,
			.mData = ca->output_buffer,
		} },
	};

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

static void aac_audio_info(void *data, struct audio_convert_info *info)
{
	UNUSED_PARAMETER(data);

	info->format = AUDIO_FORMAT_FLOAT;
}

static size_t aac_frame_size(void *data)
{
	ca_encoder *ca = data;
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
static void read_esds_desc_ext(uint8_t* desc_ext, uint8_t **buffer,
		uint32_t *size, bool version_flags)
{
	uint8_t *esds = desc_ext;
	int tag, len;
	*size = 0;

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
		if (tag == MP4DecSpecificDescrTag) {
			*buffer = bzalloc(len + 8);
			if (*buffer) {
				memcpy(*buffer, esds, len);
				*size = len;
			}
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

	uint8_t *extra_data = malloc(size);

	code = AudioConverterGetProperty(ca->converter,
			kAudioConverterCompressionMagicCookie,
			&size, extra_data);
	if (code) {
		log_osstatus(LOG_ERROR, ca,
				"AudioConverterGetProperty(magic_cookie)",
				code);
		goto free;
	}

	if (!size) {
		CA_BLOG(LOG_WARNING, "Got 0 data size for magic_cookie");
		goto free;
	}

	read_esds_desc_ext(extra_data, &ca->extra_data, &ca->extra_data_size,
			false);

free:
	free(extra_data);
}

static bool aac_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	ca_encoder *ca = data;
	
	if (!ca->extra_data)
		query_extra_data(ca);

	if (!ca->extra_data_size)
		return false;

	*extra_data = ca->extra_data;
	*size = ca->extra_data_size;
	return true;
}

static AudioConverterRef get_default_converter(UInt32 format_id)
{
	UInt32 bytes_per_frame = 8;
	UInt32 channels = 2;
	UInt32 bits_per_channel = bytes_per_frame / channels * 8;

	AudioStreamBasicDescription in = {
		.mSampleRate = 44100,
		.mChannelsPerFrame = channels,
		.mBytesPerFrame = bytes_per_frame,
		.mFramesPerPacket = 1,
		.mBytesPerPacket = 1 * bytes_per_frame,
		.mBitsPerChannel = bits_per_channel,
		.mFormatID = kAudioFormatLinearPCM,
		.mFormatFlags = kAudioFormatFlagsNativeEndian |
			kAudioFormatFlagIsPacked |
			kAudioFormatFlagIsFloat |
			0
	};

	AudioStreamBasicDescription out = {
		.mSampleRate = 44100,
		.mChannelsPerFrame = channels,
		.mBytesPerFrame = 0,
		.mFramesPerPacket = 0,
		.mBitsPerChannel = 0,
		.mFormatID = format_id,
		.mFormatFlags = 0
	};

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
	find_matching_bitrate_helper *helper = data;

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
	find_matching_bitrate_helper helper = {
		.bitrate = bitrate * 1000,
		.best_match = 0,
		.diff = INT_MAX,
	};

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
	add_bitrates_helper *helper = data;

	if (da_find(helper->bitrates, &min, 0) == DARRAY_INVALID)
		da_push_back(helper->bitrates, &min);
	if (da_find(helper->bitrates, &max, 0) == DARRAY_INVALID)
		da_push_back(helper->bitrates, &max);
}

static int bitrate_compare(const void *data1, const void *data2)
{
	const UInt32 *bitrate1 = data1;
	const UInt32 *bitrate2 = data2;

	return (int)*bitrate1 - (int)*bitrate2;
}

static void add_bitrates(obs_property_t *prop, ca_encoder *ca)
{
	add_bitrates_helper helper = { 0 };

	const size_t num_formats = ca ?
		ca->num_allowed_formats :
		sizeof(aac_formats)/sizeof(aac_formats[0]);
	const UInt32 *allowed_formats = ca ? ca->allowed_formats : aac_formats;
	for (size_t i = 0; i < num_formats; i++)
		enumerate_bitrates(ca,
				get_default_converter(allowed_formats[i]),
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
	ca_encoder *ca = data;

	obs_properties_t *props = obs_properties_create();

	obs_property_t *p = obs_properties_add_list(props, "bitrate",
			obs_module_text("Bitrate"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	add_bitrates(p, ca);

	obs_properties_add_bool(props, "allow he-aac",
			obs_module_text("AllowHEAAC"));

	return props;
}

static struct obs_encoder_info aac_info = {
	.id = "CoreAudio_AAC",
	.type = OBS_ENCODER_AUDIO,
	.codec = "AAC",
	.get_name = aac_get_name,
	.destroy = aac_destroy,
	.create = aac_create,
	.encode = aac_encode,
	.get_frame_size = aac_frame_size,
	.get_audio_info = aac_audio_info,
	.get_extra_data = aac_extra_data,
	.get_defaults = aac_defaults,
	.get_properties = aac_properties,
};

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

	obs_register_encoder(&aac_info);
	return true;
}

#ifdef _WIN32
void obs_module_unload(void)
{
	unload_core_audio();
}
#endif
