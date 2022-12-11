#include <obs-module.h>
#include <util/darray.h>
#include <util/platform.h>
#include <obs-avc.h>

#include <CoreFoundation/CoreFoundation.h>
#include <VideoToolbox/VideoToolbox.h>
#include <VideoToolbox/VTVideoEncoderList.h>
#include <CoreMedia/CoreMedia.h>

#include <util/apple/cfstring-utils.h>

#include <assert.h>

#define VT_LOG(level, format, ...) \
	blog(level, "[VideoToolbox encoder]: " format, ##__VA_ARGS__)
#define VT_LOG_ENCODER(encoder, codec_type, level, format, ...) \
	blog(level, "[VideoToolbox %s: '%s']: " format,         \
	     obs_encoder_get_name(encoder),                     \
	     codec_type_to_print_fmt(codec_type), ##__VA_ARGS__)
#define VT_BLOG(level, format, ...)                                  \
	VT_LOG_ENCODER(enc->encoder, enc->codec_type, level, format, \
		       ##__VA_ARGS__)

struct vt_encoder_type_data {
	const char *disp_name;
	const char *id;
	CMVideoCodecType codec_type;
	bool hardware_accelerated;
};

struct vt_prores_encoder_data {
	FourCharCode codec_type;
	CFStringRef encoder_id;
};
static DARRAY(struct vt_prores_encoder_data) vt_prores_hardware_encoder_list;
static DARRAY(struct vt_prores_encoder_data) vt_prores_software_encoder_list;

struct vt_encoder {
	obs_encoder_t *encoder;

	const char *vt_encoder_id;
	uint32_t width;
	uint32_t height;
	uint32_t keyint;
	uint32_t fps_num;
	uint32_t fps_den;
	const char *rate_control;
	uint32_t bitrate;
	float quality;
	bool limit_bitrate;
	uint32_t rc_max_bitrate;
	float rc_max_bitrate_window;
	const char *profile;
	CMVideoCodecType codec_type;
	bool bframes;

	int vt_pix_fmt;
	enum video_colorspace colorspace;

	VTCompressionSessionRef session;
	CMSimpleQueueRef queue;
	bool hw_enc;
	DARRAY(uint8_t) packet_data;
	DARRAY(uint8_t) extra_data;
};

static const char *codec_type_to_print_fmt(CMVideoCodecType codec_type)
{
	switch (codec_type) {
	case kCMVideoCodecType_H264:
		return "h264";
	case kCMVideoCodecType_HEVC:
		return "hevc";
	case kCMVideoCodecType_AppleProRes422Proxy:
		return "apco";
	case kCMVideoCodecType_AppleProRes422LT:
		return "apcs";
	case kCMVideoCodecType_AppleProRes422:
		return "apcn";
	case kCMVideoCodecType_AppleProRes422HQ:
		return "apch";
	default:
		return "";
	}
}

static void log_osstatus(int log_level, struct vt_encoder *enc,
			 const char *context, OSStatus code)
{
	char *c_str = NULL;
	CFErrorRef err = CFErrorCreate(kCFAllocatorDefault,
				       kCFErrorDomainOSStatus, code, NULL);
	CFStringRef str = CFErrorCopyDescription(err);

	c_str = cfstr_copy_cstr(str, kCFStringEncodingUTF8);
	if (c_str) {
		if (enc)
			VT_BLOG(log_level, "Error in %s: %s", context, c_str);
		else
			VT_LOG(log_level, "Error in %s: %s", context, c_str);
	}

	bfree(c_str);
	CFRelease(str);
	CFRelease(err);
}

static CFStringRef obs_to_vt_profile(CMVideoCodecType codec_type,
				     const char *profile,
				     enum video_format format)
{
	if (codec_type == kCMVideoCodecType_H264) {
		if (strcmp(profile, "baseline") == 0)
			return kVTProfileLevel_H264_Baseline_AutoLevel;
		else if (strcmp(profile, "main") == 0)
			return kVTProfileLevel_H264_Main_AutoLevel;
		else if (strcmp(profile, "high") == 0)
			return kVTProfileLevel_H264_High_AutoLevel;
		else
			return kVTProfileLevel_H264_Main_AutoLevel;
#ifdef ENABLE_HEVC
	} else if (codec_type == kCMVideoCodecType_HEVC) {
		if (strcmp(profile, "main") == 0) {
			if (format == VIDEO_FORMAT_P010) {
				VT_LOG(LOG_WARNING, "Forcing main10 for P010");
				return kVTProfileLevel_HEVC_Main10_AutoLevel;
			} else {
				return kVTProfileLevel_HEVC_Main_AutoLevel;
			}
		}
		if (strcmp(profile, "main10") == 0)
			return kVTProfileLevel_HEVC_Main10_AutoLevel;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 120300 // macOS 12.3
		if (__builtin_available(macOS 12.3, *)) {
			if (strcmp(profile, "main42210") == 0)
				return kVTProfileLevel_HEVC_Main42210_AutoLevel;
		}
#endif // macOS 12.3
		return kVTProfileLevel_HEVC_Main_AutoLevel;
#endif // ENABLE_HEVC
	} else {
		return kVTProfileLevel_H264_Baseline_AutoLevel;
	}
}

static CFStringRef obs_to_vt_colorspace(enum video_colorspace cs)
{
	switch (cs) {
	case VIDEO_CS_601:
		return kCVImageBufferYCbCrMatrix_ITU_R_601_4;
	case VIDEO_CS_2100_PQ:
	case VIDEO_CS_2100_HLG:
		return kCVImageBufferYCbCrMatrix_ITU_R_2020;
	default:
		return kCVImageBufferYCbCrMatrix_ITU_R_709_2;
	}
}

static CFStringRef obs_to_vt_primaries(enum video_colorspace cs)
{
	switch (cs) {
	case VIDEO_CS_601:
		return kCVImageBufferColorPrimaries_SMPTE_C;
	case VIDEO_CS_2100_PQ:
	case VIDEO_CS_2100_HLG:
		return kCVImageBufferColorPrimaries_ITU_R_2020;
	default:
		return kCVImageBufferColorPrimaries_ITU_R_709_2;
	}
}

static CFStringRef obs_to_vt_transfer(enum video_colorspace cs)
{
	switch (cs) {
	case VIDEO_CS_SRGB:
		return kCVImageBufferTransferFunction_sRGB;
	case VIDEO_CS_2100_PQ:
		return kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ;
	case VIDEO_CS_2100_HLG:
		return kCVImageBufferTransferFunction_ITU_R_2100_HLG;
	default:
		return kCVImageBufferTransferFunction_ITU_R_709_2;
	}
}

/* Adapted from Chromium GenerateMasteringDisplayColorVolume */
static CFDataRef obs_to_vt_masteringdisplay(uint32_t hdr_nominal_peak_level)
{
	struct mastering_display_colour_volume {
		uint16_t display_primaries[3][2];
		uint16_t white_point[2];
		uint32_t max_display_mastering_luminance;
		uint32_t min_display_mastering_luminance;
	};
	static_assert(sizeof(struct mastering_display_colour_volume) == 24,
		      "May need to adjust struct packing");

	struct mastering_display_colour_volume mdcv;
	mdcv.display_primaries[0][0] = __builtin_bswap16(13250);
	mdcv.display_primaries[0][1] = __builtin_bswap16(34500);
	mdcv.display_primaries[1][0] = __builtin_bswap16(7500);
	mdcv.display_primaries[1][1] = __builtin_bswap16(3000);
	mdcv.display_primaries[2][0] = __builtin_bswap16(34000);
	mdcv.display_primaries[2][1] = __builtin_bswap16(16000);
	mdcv.white_point[0] = __builtin_bswap16(15635);
	mdcv.white_point[1] = __builtin_bswap16(16450);
	mdcv.max_display_mastering_luminance =
		__builtin_bswap32(hdr_nominal_peak_level * 10000);
	mdcv.min_display_mastering_luminance = 0;

	UInt8 bytes[sizeof(struct mastering_display_colour_volume)];
	memcpy(bytes, &mdcv, sizeof(bytes));
	return CFDataCreate(NULL, bytes, sizeof(bytes));
}

/* Adapted from Chromium GenerateContentLightLevelInfo */
static CFDataRef
obs_to_vt_contentlightlevelinfo(uint16_t hdr_nominal_peak_level)
{
	struct content_light_level_info {
		uint16_t max_content_light_level;
		uint16_t max_pic_average_light_level;
	};
	static_assert(sizeof(struct content_light_level_info) == 4,
		      "May need to adjust struct packing");

	struct content_light_level_info clli;
	clli.max_content_light_level =
		__builtin_bswap16(hdr_nominal_peak_level);
	clli.max_pic_average_light_level =
		__builtin_bswap16(hdr_nominal_peak_level);

	UInt8 bytes[sizeof(struct content_light_level_info)];
	memcpy(bytes, &clli, sizeof(bytes));
	return CFDataCreate(NULL, bytes, sizeof(bytes));
}

#define STATUS_CHECK(c)                                 \
	code = c;                                       \
	if (code) {                                     \
		log_osstatus(LOG_ERROR, enc, #c, code); \
		goto fail;                              \
	}

#define SESSION_CHECK(x)           \
	if ((code = (x)) != noErr) \
		return code;

static OSStatus session_set_prop_float(VTCompressionSessionRef session,
				       CFStringRef key, float val)
{
	CFNumberRef n = CFNumberCreate(NULL, kCFNumberFloat32Type, &val);
	OSStatus code = VTSessionSetProperty(session, key, n);
	CFRelease(n);

	return code;
}

static OSStatus session_set_prop_int(VTCompressionSessionRef session,
				     CFStringRef key, int32_t val)
{
	CFNumberRef n = CFNumberCreate(NULL, kCFNumberSInt32Type, &val);
	OSStatus code = VTSessionSetProperty(session, key, n);
	CFRelease(n);

	return code;
}

static OSStatus session_set_prop_str(VTCompressionSessionRef session,
				     CFStringRef key, char *val)
{
	CFStringRef s = CFStringCreateWithFileSystemRepresentation(NULL, val);
	OSStatus code = VTSessionSetProperty(session, key, s);
	CFRelease(s);

	return code;
}

static OSStatus session_set_prop(VTCompressionSessionRef session,
				 CFStringRef key, CFTypeRef val)
{
	return VTSessionSetProperty(session, key, val);
}

static OSStatus session_set_bitrate(VTCompressionSessionRef session,
				    const char *rate_control, int new_bitrate,
				    float quality, bool limit_bitrate,
				    int max_bitrate, float max_bitrate_window)
{
	OSStatus code;

	bool can_limit_bitrate;
	CFStringRef compressionPropertyKey;

	if (strcmp(rate_control, "CBR") == 0) {
		compressionPropertyKey =
			kVTCompressionPropertyKey_AverageBitRate;
		can_limit_bitrate = true;

		if (__builtin_available(macOS 13.0, *)) {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 130000
#ifdef __aarch64__
			if (true) {
#else
			if (os_get_emulation_status() == true) {
#endif
				compressionPropertyKey =
					kVTCompressionPropertyKey_ConstantBitRate;
				can_limit_bitrate = false;
			} else {
				VT_LOG(LOG_WARNING,
				       "CBR support for VideoToolbox encoder requires Apple Silicon. "
				       "Will use ABR instead.");
			}
#else
			VT_LOG(LOG_WARNING,
			       "CBR support for VideoToolbox not available in this build of OBS. "
			       "Will use ABR instead.");
#endif
		} else {
			VT_LOG(LOG_WARNING,
			       "CBR support for VideoToolbox encoder requires macOS 13 or newer. "
			       "Will use ABR instead.");
		}
	} else if (strcmp(rate_control, "ABR") == 0) {
		compressionPropertyKey =
			kVTCompressionPropertyKey_AverageBitRate;
		can_limit_bitrate = true;
	} else if (strcmp(rate_control, "CRF") == 0) {
#ifdef __aarch64__
		if (true) {
#else
		if (os_get_emulation_status() == true) {
#endif
			compressionPropertyKey =
				kVTCompressionPropertyKey_Quality;
			SESSION_CHECK(session_set_prop_float(
				session, compressionPropertyKey, quality));
		} else {
			VT_LOG(LOG_WARNING,
			       "CRF support for VideoToolbox encoder requires Apple Silicon. "
			       "Will use ABR instead.");
			compressionPropertyKey =
				kVTCompressionPropertyKey_AverageBitRate;
		}
		can_limit_bitrate = true;
	} else {
		VT_LOG(LOG_ERROR,
		       "Selected rate control method is not supported: %s",
		       rate_control);
		return kVTParameterErr;
	}

	if (compressionPropertyKey != kVTCompressionPropertyKey_Quality) {
		SESSION_CHECK(session_set_prop_int(
			session, compressionPropertyKey, new_bitrate * 1000));
	}

	if (limit_bitrate && can_limit_bitrate) {
		int32_t cpb_size = max_bitrate * 125 * max_bitrate_window;

		CFNumberRef cf_cpb_size =
			CFNumberCreate(NULL, kCFNumberIntType, &cpb_size);
		CFNumberRef cf_cpb_window_s = CFNumberCreate(
			NULL, kCFNumberFloatType, &max_bitrate_window);

		CFMutableArrayRef rate_control = CFArrayCreateMutable(
			kCFAllocatorDefault, 2, &kCFTypeArrayCallBacks);

		CFArrayAppendValue(rate_control, cf_cpb_size);
		CFArrayAppendValue(rate_control, cf_cpb_window_s);

		code = session_set_prop(
			session, kVTCompressionPropertyKey_DataRateLimits,
			rate_control);

		CFRelease(cf_cpb_size);
		CFRelease(cf_cpb_window_s);
		CFRelease(rate_control);

		if (code == kVTPropertyNotSupportedErr) {
			log_osstatus(LOG_WARNING, NULL,
				     "setting DataRateLimits on session", code);
			return noErr;
		}
	}

	return noErr;
}

static OSStatus session_set_colorspace(VTCompressionSessionRef session,
				       enum video_colorspace cs)
{
	OSStatus code;
	SESSION_CHECK(session_set_prop(session,
				       kVTCompressionPropertyKey_ColorPrimaries,
				       obs_to_vt_primaries(cs)));
	SESSION_CHECK(session_set_prop(
		session, kVTCompressionPropertyKey_TransferFunction,
		obs_to_vt_transfer(cs)));
	SESSION_CHECK(session_set_prop(session,
				       kVTCompressionPropertyKey_YCbCrMatrix,
				       obs_to_vt_colorspace(cs)));
	const bool pq = cs == VIDEO_CS_2100_PQ;
	const bool hlg = cs == VIDEO_CS_2100_HLG;
	if (pq || hlg) {
		const uint16_t hdr_nominal_peak_level =
			pq ? (uint16_t)obs_get_video_hdr_nominal_peak_level()
			   : (hlg ? 1000 : 0);
		SESSION_CHECK(session_set_prop(
			session,
			kVTCompressionPropertyKey_MasteringDisplayColorVolume,
			obs_to_vt_masteringdisplay(hdr_nominal_peak_level)));
		SESSION_CHECK(session_set_prop(
			session,
			kVTCompressionPropertyKey_ContentLightLevelInfo,
			obs_to_vt_contentlightlevelinfo(
				hdr_nominal_peak_level)));
	}

	return noErr;
}

#undef SESSION_CHECK

void sample_encoded_callback(void *data, void *source, OSStatus status,
			     VTEncodeInfoFlags info_flags,
			     CMSampleBufferRef buffer)
{
	UNUSED_PARAMETER(status);
	UNUSED_PARAMETER(info_flags);

	CMSimpleQueueRef queue = data;
	CVPixelBufferRef pixbuf = source;
	if (buffer != NULL) {
		CFRetain(buffer);
		CMSimpleQueueEnqueue(queue, buffer);
	}
	CFRelease(pixbuf);
}
#define ENCODER_ID kVTVideoEncoderSpecification_EncoderID
static inline CFMutableDictionaryRef
create_encoder_spec(const char *vt_encoder_id)
{
	CFMutableDictionaryRef encoder_spec = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);

	CFStringRef id =
		CFStringCreateWithFileSystemRepresentation(NULL, vt_encoder_id);
	CFDictionaryAddValue(encoder_spec, ENCODER_ID, id);
	CFRelease(id);

	return encoder_spec;
}

static inline CFMutableDictionaryRef
create_prores_encoder_spec(CMVideoCodecType target_codec_type,
			   bool hardware_accelerated)
{
	CFStringRef encoder_id = NULL;

	size_t size = 0;
	struct vt_prores_encoder_data *encoder_list = NULL;
	if (hardware_accelerated) {
		size = vt_prores_hardware_encoder_list.num;
		encoder_list = vt_prores_hardware_encoder_list.array;
	} else {
		size = vt_prores_software_encoder_list.num;
		encoder_list = vt_prores_software_encoder_list.array;
	}

	for (size_t i = 0; i < size; ++i) {
		if (target_codec_type == encoder_list[i].codec_type) {
			encoder_id = encoder_list[i].encoder_id;
		}
	}

	CFMutableDictionaryRef encoder_spec = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);

	CFDictionaryAddValue(encoder_spec, ENCODER_ID, encoder_id);

	return encoder_spec;
}
#undef ENCODER_ID

static inline CFMutableDictionaryRef create_pixbuf_spec(struct vt_encoder *enc)
{
	CFMutableDictionaryRef pixbuf_spec = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);

	CFNumberRef n =
		CFNumberCreate(NULL, kCFNumberSInt32Type, &enc->vt_pix_fmt);
	CFDictionaryAddValue(pixbuf_spec, kCVPixelBufferPixelFormatTypeKey, n);
	CFRelease(n);

	n = CFNumberCreate(NULL, kCFNumberSInt32Type, &enc->width);
	CFDictionaryAddValue(pixbuf_spec, kCVPixelBufferWidthKey, n);
	CFRelease(n);

	n = CFNumberCreate(NULL, kCFNumberSInt32Type, &enc->height);
	CFDictionaryAddValue(pixbuf_spec, kCVPixelBufferHeightKey, n);
	CFRelease(n);

	return pixbuf_spec;
}

static bool create_encoder(struct vt_encoder *enc)
{
	OSStatus code;

	VTCompressionSessionRef s;

	const char *codec_name = obs_encoder_get_codec(enc->encoder);

	CFDictionaryRef encoder_spec;
	if (strcmp(codec_name, "prores") == 0) {
		struct vt_encoder_type_data *type_data =
			(struct vt_encoder_type_data *)
				obs_encoder_get_type_data(enc->encoder);
		encoder_spec = create_prores_encoder_spec(
			enc->codec_type, type_data->hardware_accelerated);
	} else {
		encoder_spec = create_encoder_spec(enc->vt_encoder_id);
	}

	CFDictionaryRef pixbuf_spec = create_pixbuf_spec(enc);

	STATUS_CHECK(VTCompressionSessionCreate(
		kCFAllocatorDefault, enc->width, enc->height, enc->codec_type,
		encoder_spec, pixbuf_spec, NULL, &sample_encoded_callback,
		enc->queue, &s));

	CFRelease(encoder_spec);
	CFRelease(pixbuf_spec);

	CFBooleanRef b = NULL;
	code = VTSessionCopyProperty(
		s,
		kVTCompressionPropertyKey_UsingHardwareAcceleratedVideoEncoder,
		NULL, &b);

	if (code == noErr && (enc->hw_enc = CFBooleanGetValue(b)))
		VT_BLOG(LOG_INFO, "session created with hardware encoding");
	else
		enc->hw_enc = false;

	if (b != NULL)
		CFRelease(b);

	if (enc->codec_type == kCMVideoCodecType_H264 ||
	    enc->codec_type == kCMVideoCodecType_HEVC) {

		// This can fail when using GPU hardware encoding
		code = session_set_prop_int(
			s,
			kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration,
			enc->keyint);
		if (code != noErr)
			log_osstatus(
				LOG_WARNING, enc,
				"setting kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration failed, "
				"keyframe interval might be incorrect",
				code);

		STATUS_CHECK(session_set_prop_int(
			s, kVTCompressionPropertyKey_MaxKeyFrameInterval,
			enc->keyint * ((float)enc->fps_num / enc->fps_den)));
		STATUS_CHECK(session_set_prop_float(
			s, kVTCompressionPropertyKey_ExpectedFrameRate,
			(float)enc->fps_num / enc->fps_den));
		STATUS_CHECK(session_set_prop(
			s, kVTCompressionPropertyKey_AllowFrameReordering,
			enc->bframes ? kCFBooleanTrue : kCFBooleanFalse));

		video_t *video = obs_encoder_video(enc->encoder);
		const struct video_output_info *voi =
			video_output_get_info(video);
		STATUS_CHECK(session_set_prop(
			s, kVTCompressionPropertyKey_ProfileLevel,
			obs_to_vt_profile(enc->codec_type, enc->profile,
					  voi->format)));
		STATUS_CHECK(session_set_bitrate(
			s, enc->rate_control, enc->bitrate, enc->quality,
			enc->limit_bitrate, enc->rc_max_bitrate,
			enc->rc_max_bitrate_window));
	}

	// This can fail depending on hardware configuration
	code = session_set_prop(s, kVTCompressionPropertyKey_RealTime,
				kCFBooleanFalse);
	if (code != noErr)
		log_osstatus(
			LOG_WARNING, enc,
			"setting kVTCompressionPropertyKey_RealTime failed, "
			"frame delay might be increased",
			code);

	STATUS_CHECK(session_set_colorspace(s, enc->colorspace));

	STATUS_CHECK(VTCompressionSessionPrepareToEncodeFrames(s));

	enc->session = s;

	return true;

fail:
	if (encoder_spec != NULL)
		CFRelease(encoder_spec);
	if (pixbuf_spec != NULL)
		CFRelease(pixbuf_spec);

	return false;
}

static void vt_destroy(void *data)
{
	struct vt_encoder *enc = data;

	if (enc) {
		if (enc->session != NULL) {
			VTCompressionSessionInvalidate(enc->session);
			CFRelease(enc->session);
		}
		da_free(enc->packet_data);
		da_free(enc->extra_data);
		bfree(enc);
	}
}

static void dump_encoder_info(struct vt_encoder *enc)
{
	VT_BLOG(LOG_INFO,
		"settings:\n"
		"\tvt_encoder_id          %s\n"
		"\trate_control:          %s\n"
		"\tbitrate:               %d (kbps)\n"
		"\tquality:               %f\n"
		"\tfps_num:               %d\n"
		"\tfps_den:               %d\n"
		"\twidth:                 %d\n"
		"\theight:                %d\n"
		"\tkeyint:                %d (s)\n"
		"\tlimit_bitrate:         %s\n"
		"\trc_max_bitrate:        %d (kbps)\n"
		"\trc_max_bitrate_window: %f (s)\n"
		"\thw_enc:                %s\n"
		"\tprofile:               %s\n"
		"\tcodec_type:            %.4s\n",
		enc->vt_encoder_id, enc->rate_control, enc->bitrate,
		enc->quality, enc->fps_num, enc->fps_den, enc->width,
		enc->height, enc->keyint, enc->limit_bitrate ? "on" : "off",
		enc->rc_max_bitrate, enc->rc_max_bitrate_window,
		enc->hw_enc ? "on" : "off",
		(enc->profile != NULL && !!strlen(enc->profile)) ? enc->profile
								 : "default",
		codec_type_to_print_fmt(enc->codec_type));
}

static bool set_video_format(struct vt_encoder *enc, enum video_format format,
			     enum video_range_type range)
{
	bool full_range = range == VIDEO_RANGE_FULL;
	switch (format) {
	case VIDEO_FORMAT_I420:
		enc->vt_pix_fmt =
			full_range
				? kCVPixelFormatType_420YpCbCr8PlanarFullRange
				: kCVPixelFormatType_420YpCbCr8Planar;
		return true;
	case VIDEO_FORMAT_NV12:
		enc->vt_pix_fmt =
			full_range
				? kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
				: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
		return true;
	case VIDEO_FORMAT_P010:
		if (enc->codec_type == kCMVideoCodecType_HEVC) {
			enc->vt_pix_fmt =
				full_range
					? kCVPixelFormatType_420YpCbCr10BiPlanarFullRange
					: kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange;
			return true;
		} else {
			return false;
		}
	default:
		return false;
	}
	return false;
}

static bool update_params(struct vt_encoder *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	const char *codec = obs_encoder_get_codec(enc->encoder);
	if (strcmp(codec, "h264") == 0) {
		enc->codec_type = kCMVideoCodecType_H264;
		obs_data_set_int(settings, "codec_type", enc->codec_type);
#ifdef ENABLE_HEVC
	} else if (strcmp(codec, "hevc") == 0) {
		enc->codec_type = kCMVideoCodecType_HEVC;
		obs_data_set_int(settings, "codec_type", enc->codec_type);
#endif
	} else {
		enc->codec_type = (CMVideoCodecType)obs_data_get_int(
			settings, "codec_type");
	}

	if (!set_video_format(enc, voi->format, voi->range)) {
		obs_encoder_set_last_error(
			enc->encoder,
			obs_module_text("ColorFormatUnsupported"));
		VT_BLOG(LOG_WARNING, "Unsupported color format selected");
		return false;
	}
	enc->colorspace = voi->colorspace;
	enc->width = obs_encoder_get_width(enc->encoder);
	enc->height = obs_encoder_get_height(enc->encoder);
	enc->fps_num = voi->fps_num;
	enc->fps_den = voi->fps_den;
	enc->keyint = (uint32_t)obs_data_get_int(settings, "keyint_sec");
	enc->rate_control = obs_data_get_string(settings, "rate_control");
	enc->bitrate = (uint32_t)obs_data_get_int(settings, "bitrate");
	enc->quality = ((float)obs_data_get_int(settings, "quality")) / 100;
	enc->profile = obs_data_get_string(settings, "profile");
	enc->limit_bitrate = obs_data_get_bool(settings, "limit_bitrate");
	enc->rc_max_bitrate = obs_data_get_int(settings, "max_bitrate");
	enc->rc_max_bitrate_window =
		obs_data_get_double(settings, "max_bitrate_window");
	enc->bframes = obs_data_get_bool(settings, "bframes");

	return true;
}

static bool vt_update(void *data, obs_data_t *settings)
{
	struct vt_encoder *enc = data;

	uint32_t old_bitrate = enc->bitrate;
	bool old_limit_bitrate = enc->limit_bitrate;

	update_params(enc, settings);

	if (old_bitrate == enc->bitrate &&
	    old_limit_bitrate == enc->limit_bitrate)
		return true;

	OSStatus code = session_set_bitrate(enc->session, enc->rate_control,
					    enc->bitrate, enc->quality,
					    enc->limit_bitrate,
					    enc->rc_max_bitrate,
					    enc->rc_max_bitrate_window);
	if (code != noErr)
		VT_BLOG(LOG_WARNING, "Failed to set bitrate to session");

	dump_encoder_info(enc);
	return true;
}

static void *vt_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	struct vt_encoder *enc = bzalloc(sizeof(struct vt_encoder));

	OSStatus code;

	enc->encoder = encoder;
	enc->vt_encoder_id = obs_encoder_get_id(encoder);

	if (!update_params(enc, settings))
		goto fail;

	STATUS_CHECK(CMSimpleQueueCreate(NULL, 100, &enc->queue));

	if (!create_encoder(enc))
		goto fail;

	dump_encoder_info(enc);

	return enc;

fail:
	vt_destroy(enc);
	return NULL;
}

static const uint8_t annexb_startcode[4] = {0, 0, 0, 1};

static void packet_put(struct darray *packet, const uint8_t *buf, size_t size)
{
	darray_push_back_array(sizeof(uint8_t), packet, buf, size);
}

static void packet_put_startcode(struct darray *packet, int size)
{
	assert(size == 3 || size == 4);

	packet_put(packet, &annexb_startcode[4 - size], size);
}

static bool handle_prores_packet(struct vt_encoder *enc,
				 CMSampleBufferRef buffer)
{
	OSStatus err = 0;
	size_t block_size = 0;
	uint8_t *block_buf = NULL;

	CMBlockBufferRef block = CMSampleBufferGetDataBuffer(buffer);
	if (block == NULL) {
		VT_BLOG(LOG_ERROR,
			"Failed to get block buffer for ProRes frame.");
		return false;
	}
	err = CMBlockBufferGetDataPointer(block, 0, NULL, &block_size,
					  (char **)&block_buf);
	if (err != 0) {
		VT_BLOG(LOG_ERROR,
			"Failed to get data buffer pointer for ProRes frame.");
		return false;
	}

	packet_put(&enc->packet_data.da, block_buf, block_size);

	return true;
}

static void convert_block_nals_to_annexb(struct vt_encoder *enc,
					 struct darray *packet,
					 CMBlockBufferRef block,
					 int nal_length_bytes)
{
	size_t block_size;
	uint8_t *block_buf;

	CMBlockBufferGetDataPointer(block, 0, NULL, &block_size,
				    (char **)&block_buf);

	size_t bytes_remaining = block_size;

	while (bytes_remaining > 0) {
		uint32_t nal_size;
		if (nal_length_bytes == 1)
			nal_size = block_buf[0];
		else if (nal_length_bytes == 2)
			nal_size = CFSwapInt16BigToHost(
				((uint16_t *)block_buf)[0]);
		else if (nal_length_bytes == 4)
			nal_size = CFSwapInt32BigToHost(
				((uint32_t *)block_buf)[0]);
		else
			return;

		bytes_remaining -= nal_length_bytes;
		block_buf += nal_length_bytes;

		if (bytes_remaining < nal_size) {
			VT_BLOG(LOG_ERROR, "invalid nal block");
			return;
		}

		packet_put_startcode(packet, 3);
		packet_put(packet, block_buf, nal_size);

		bytes_remaining -= nal_size;
		block_buf += nal_size;
	}
}

static bool handle_keyframe(struct vt_encoder *enc,
			    CMFormatDescriptionRef format_desc,
			    size_t param_count, struct darray *packet,
			    struct darray *extra_data)
{
	OSStatus code;
	const uint8_t *param;
	size_t param_size;

	for (size_t i = 0; i < param_count; i++) {
		if (enc->codec_type == kCMVideoCodecType_H264) {
			code = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
				format_desc, i, &param, &param_size, NULL,
				NULL);
#ifdef ENABLE_HEVC
		} else if (enc->codec_type == kCMVideoCodecType_HEVC) {
			code = CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(
				format_desc, i, &param, &param_size, NULL,
				NULL);
#endif
		}
		if (code != noErr) {
			log_osstatus(LOG_ERROR, enc,
				     "getting NAL parameter "
				     "at index",
				     code);
			return false;
		}

		packet_put_startcode(packet, 4);
		packet_put(packet, param, param_size);
	}

	// if we were passed an extra_data array, fill it with
	// SPS, PPS, etc.
	if (extra_data != NULL)
		packet_put(extra_data, packet->array, packet->num);

	return true;
}

static bool convert_sample_to_annexb(struct vt_encoder *enc,
				     struct darray *packet,
				     struct darray *extra_data,
				     CMSampleBufferRef buffer, bool keyframe)
{
	OSStatus code;
	CMFormatDescriptionRef format_desc =
		CMSampleBufferGetFormatDescription(buffer);

	size_t param_count;
	int nal_length_bytes;
	if (enc->codec_type == kCMVideoCodecType_H264) {
		code = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
			format_desc, 0, NULL, NULL, &param_count,
			&nal_length_bytes);
#ifdef ENABLE_HEVC
	} else if (enc->codec_type == kCMVideoCodecType_HEVC) {
		code = CMVideoFormatDescriptionGetHEVCParameterSetAtIndex(
			format_desc, 0, NULL, NULL, &param_count,
			&nal_length_bytes);
#endif
	}
	// it is not clear what errors this function can return
	// so we check the two most reasonable
	if (code == kCMFormatDescriptionBridgeError_InvalidParameter ||
	    code == kCMFormatDescriptionError_InvalidParameter) {
		VT_BLOG(LOG_WARNING, "assuming 2 parameter sets "
				     "and 4 byte NAL length header");
		param_count = 2;
		nal_length_bytes = 4;

	} else if (code != noErr) {
		log_osstatus(LOG_ERROR, enc,
			     "getting parameter count from sample", code);
		return false;
	}

	if (keyframe &&
	    !handle_keyframe(enc, format_desc, param_count, packet, extra_data))
		return false;

	CMBlockBufferRef block = CMSampleBufferGetDataBuffer(buffer);
	convert_block_nals_to_annexb(enc, packet, block, nal_length_bytes);

	return true;
}

static bool is_sample_keyframe(CMSampleBufferRef buffer)
{
	CFArrayRef attachments =
		CMSampleBufferGetSampleAttachmentsArray(buffer, false);
	if (attachments != NULL) {
		CFDictionaryRef attachment;
		CFBooleanRef has_dependencies;
		attachment =
			(CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
		has_dependencies = (CFBooleanRef)CFDictionaryGetValue(
			attachment, kCMSampleAttachmentKey_DependsOnOthers);
		return has_dependencies == kCFBooleanFalse;
	}

	return false;
}

static bool parse_sample(struct vt_encoder *enc, CMSampleBufferRef buffer,
			 struct encoder_packet *packet, CMTime off)
{
	CMTime pts = CMSampleBufferGetPresentationTimeStamp(buffer);
	CMTime dts = CMSampleBufferGetDecodeTimeStamp(buffer);

	if (CMTIME_IS_INVALID(dts))
		dts = pts;
	// imitate x264's negative dts when bframes might have pts < dts
	else if (enc->bframes)
		dts = CMTimeSubtract(dts, off);

	pts = CMTimeMultiply(pts, enc->fps_num);
	dts = CMTimeMultiply(dts, enc->fps_num);

	const bool is_avc = enc->codec_type == kCMVideoCodecType_H264;
	const bool has_annexb = is_avc ||
				(enc->codec_type == kCMVideoCodecType_HEVC);

	// All ProRes frames are "keyframes"
	const bool keyframe = !has_annexb || is_sample_keyframe(buffer);

	da_resize(enc->packet_data, 0);

	// If we are still looking for extra data
	struct darray *extra_data = NULL;
	if (enc->extra_data.num == 0)
		extra_data = &enc->extra_data.da;

	if (has_annexb) {
		if (!convert_sample_to_annexb(enc, &enc->packet_data.da,
					      extra_data, buffer, keyframe))
			goto fail;
	} else {
		if (!handle_prores_packet(enc, buffer))
			goto fail;
	}

	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = (int64_t)(CMTimeGetSeconds(pts));
	packet->dts = (int64_t)(CMTimeGetSeconds(dts));
	packet->data = enc->packet_data.array;
	packet->size = enc->packet_data.num;
	packet->keyframe = keyframe;

	if (is_avc) {
		// VideoToolbox produces packets with priority lower than the RTMP code
		// expects, which causes it to be unable to recover from frame drops.
		// Fix this by manually adjusting the priority.
		uint8_t *start = enc->packet_data.array;
		uint8_t *end = start + enc->packet_data.num;

		start = (uint8_t *)obs_avc_find_startcode(start, end);
		while (true) {
			while (start < end && !*(start++))
				;

			if (start == end)
				break;

			const int type = start[0] & 0x1F;
			if (type == OBS_NAL_SLICE_IDR ||
			    type == OBS_NAL_SLICE) {
				uint8_t prev_type = (start[0] >> 5) & 0x3;
				start[0] &= ~(3 << 5);

				if (type == OBS_NAL_SLICE_IDR)
					start[0] |= OBS_NAL_PRIORITY_HIGHEST
						    << 5;
				else if (type == OBS_NAL_SLICE &&
					 prev_type !=
						 OBS_NAL_PRIORITY_DISPOSABLE)
					start[0] |= OBS_NAL_PRIORITY_HIGH << 5;
				else
					start[0] |= prev_type << 5;
			}

			start = (uint8_t *)obs_avc_find_startcode(start, end);
		}
	}

	CFRelease(buffer);
	return true;

fail:
	CFRelease(buffer);
	return false;
}

bool get_cached_pixel_buffer(struct vt_encoder *enc, CVPixelBufferRef *buf)
{
	OSStatus code;
	CVPixelBufferPoolRef pool =
		VTCompressionSessionGetPixelBufferPool(enc->session);
	if (!pool)
		return kCVReturnError;

	CVPixelBufferRef pixbuf;
	STATUS_CHECK(CVPixelBufferPoolCreatePixelBuffer(NULL, pool, &pixbuf));

	// Why aren't these already set on the pixel buffer?
	// I would have expected pixel buffers from the session's
	// pool to have the correct color space stuff set

	const enum video_colorspace cs = enc->colorspace;
	CVBufferSetAttachment(pixbuf, kCVImageBufferYCbCrMatrixKey,
			      obs_to_vt_colorspace(cs),
			      kCVAttachmentMode_ShouldPropagate);
	CVBufferSetAttachment(pixbuf, kCVImageBufferColorPrimariesKey,
			      obs_to_vt_primaries(cs),
			      kCVAttachmentMode_ShouldPropagate);
	CVBufferSetAttachment(pixbuf, kCVImageBufferTransferFunctionKey,
			      obs_to_vt_transfer(cs),
			      kCVAttachmentMode_ShouldPropagate);
	const bool pq = cs == VIDEO_CS_2100_PQ;
	const bool hlg = cs == VIDEO_CS_2100_HLG;
	if (pq || hlg) {
		const uint16_t hdr_nominal_peak_level =
			pq ? (uint16_t)obs_get_video_hdr_nominal_peak_level()
			   : (hlg ? 1000 : 0);
		CVBufferSetAttachment(
			pixbuf, kCVImageBufferMasteringDisplayColorVolumeKey,
			obs_to_vt_masteringdisplay(hdr_nominal_peak_level),
			kCVAttachmentMode_ShouldPropagate);
		CVBufferSetAttachment(
			pixbuf, kCVImageBufferContentLightLevelInfoKey,
			obs_to_vt_contentlightlevelinfo(hdr_nominal_peak_level),
			kCVAttachmentMode_ShouldPropagate);
	}

	*buf = pixbuf;
	return true;

fail:
	return false;
}

static bool vt_encode(void *data, struct encoder_frame *frame,
		      struct encoder_packet *packet, bool *received_packet)
{
	struct vt_encoder *enc = data;

	OSStatus code;

	CMTime dur = CMTimeMake(enc->fps_den, enc->fps_num);
	CMTime off = CMTimeMultiply(dur, 2);
	CMTime pts = CMTimeMake(frame->pts, enc->fps_num);

	CVPixelBufferRef pixbuf = NULL;

	if (!get_cached_pixel_buffer(enc, &pixbuf)) {
		VT_BLOG(LOG_ERROR, "Unable to create pixel buffer");
		goto fail;
	}

	STATUS_CHECK(CVPixelBufferLockBaseAddress(pixbuf, 0));

	for (int i = 0; i < MAX_AV_PLANES; i++) {
		if (frame->data[i] == NULL)
			break;
		uint8_t *p = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(
			pixbuf, i);
		uint8_t *f = frame->data[i];
		size_t plane_linesize =
			CVPixelBufferGetBytesPerRowOfPlane(pixbuf, i);
		size_t plane_height = CVPixelBufferGetHeightOfPlane(pixbuf, i);

		for (size_t j = 0; j < plane_height; j++) {
			memcpy(p, f, frame->linesize[i]);
			p += plane_linesize;
			f += frame->linesize[i];
		}
	}

	STATUS_CHECK(CVPixelBufferUnlockBaseAddress(pixbuf, 0));

	STATUS_CHECK(VTCompressionSessionEncodeFrame(enc->session, pixbuf, pts,
						     dur, NULL, pixbuf, NULL));

	CMSampleBufferRef buffer =
		(CMSampleBufferRef)CMSimpleQueueDequeue(enc->queue);

	// No samples waiting in the queue
	if (buffer == NULL)
		return true;

	*received_packet = true;
	return parse_sample(enc, buffer, packet, off);

fail:
	return false;
}

#undef STATUS_CHECK
#undef CFNUM_INT

static bool vt_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct vt_encoder *enc = (struct vt_encoder *)data;
	*extra_data = enc->extra_data.array;
	*size = enc->extra_data.num;
	return true;
}

static const char *vt_getname(void *data)
{
	struct vt_encoder_type_data *type_data = data;

	if (strcmp("Apple H.264 (HW)", type_data->disp_name) == 0) {
		return obs_module_text("VTH264EncHW");
	} else if (strcmp("Apple H.264 (SW)", type_data->disp_name) == 0) {
		return obs_module_text("VTH264EncSW");
#ifdef ENABLE_HEVC
	} else if (strcmp("Apple HEVC (HW)", type_data->disp_name) == 0) {
		return obs_module_text("VTHEVCEncHW");
	} else if (strcmp("Apple HEVC (AVE)", type_data->disp_name) == 0) {
		return obs_module_text("VTHEVCEncT2");
	} else if (strcmp("Apple HEVC (SW)", type_data->disp_name) == 0) {
		return obs_module_text("VTHEVCEncSW");
#endif
	} else if (strncmp("AppleProResHW", type_data->disp_name, 13) == 0) {
		return obs_module_text("VTProResEncHW");
	} else if (strncmp("Apple ProRes", type_data->disp_name, 12) == 0) {
		return obs_module_text("VTProResEncSW");
	}
	return type_data->disp_name;
}

#define TEXT_BITRATE obs_module_text("Bitrate")
#define TEXT_QUALITY obs_module_text("Quality")
#define TEXT_USE_MAX_BITRATE obs_module_text("UseMaxBitrate")
#define TEXT_MAX_BITRATE obs_module_text("MaxBitrate")
#define TEXT_MAX_BITRATE_WINDOW obs_module_text("MaxBitrateWindow")
#define TEXT_KEYINT_SEC obs_module_text("KeyframeIntervalSec")
#define TEXT_PROFILE obs_module_text("Profile")
#define TEXT_BFRAMES obs_module_text("UseBFrames")
#define TEXT_RATE_CONTROL obs_module_text("RateControl")
#define TEXT_PRORES_CODEC obs_module_text("ProResCodec")

static bool rate_control_limit_bitrate_modified(obs_properties_t *ppts,
						obs_property_t *p,
						obs_data_t *settings)
{
	bool has_bitrate = true;
	bool can_limit_bitrate = true;
	bool use_limit_bitrate = obs_data_get_bool(settings, "limit_bitrate");
	const char *rate_control =
		obs_data_get_string(settings, "rate_control");

	if (strcmp(rate_control, "CBR") == 0) {
		can_limit_bitrate = false;
		has_bitrate = true;
	} else if (strcmp(rate_control, "CRF") == 0) {
		can_limit_bitrate = true;
		has_bitrate = false;
	} else if (strcmp(rate_control, "ABR") == 0) {
		can_limit_bitrate = true;
		has_bitrate = true;
	}

	p = obs_properties_get(ppts, "limit_bitrate");
	obs_property_set_visible(p, can_limit_bitrate);
	p = obs_properties_get(ppts, "max_bitrate");
	obs_property_set_visible(p, can_limit_bitrate && use_limit_bitrate);
	p = obs_properties_get(ppts, "max_bitrate_window");
	obs_property_set_visible(p, can_limit_bitrate && use_limit_bitrate);

	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, has_bitrate);
	p = obs_properties_get(ppts, "quality");
	obs_property_set_visible(p, !has_bitrate);
	return true;
}

static obs_properties_t *vt_properties_h26x(void *unused, void *data)
{
	UNUSED_PARAMETER(unused);
	struct vt_encoder_type_data *type_data = data;

	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(props, "rate_control", TEXT_RATE_CONTROL,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	if (__builtin_available(macOS 13.0, *))
		if (type_data->hardware_accelerated
#ifndef __aarch64__
		    && (os_get_emulation_status() == true)
#endif
		)
			obs_property_list_add_string(p, "CBR", "CBR");
	obs_property_list_add_string(p, "ABR", "ABR");
	if (type_data->hardware_accelerated
#ifndef __aarch64__
	    && (os_get_emulation_status() == true)
#endif
	)
		obs_property_list_add_string(p, "CRF", "CRF");
	obs_property_set_modified_callback(p,
					   rate_control_limit_bitrate_modified);

	p = obs_properties_add_int(props, "bitrate", TEXT_BITRATE, 50, 10000000,
				   50);
	obs_property_int_set_suffix(p, " Kbps");
	obs_properties_add_int_slider(props, "quality", TEXT_QUALITY, 0, 100,
				      1);

	p = obs_properties_add_bool(props, "limit_bitrate",
				    TEXT_USE_MAX_BITRATE);
	obs_property_set_modified_callback(p,
					   rate_control_limit_bitrate_modified);

	p = obs_properties_add_int(props, "max_bitrate", TEXT_MAX_BITRATE, 50,
				   10000000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	p = obs_properties_add_float(props, "max_bitrate_window",
				     TEXT_MAX_BITRATE_WINDOW, 0.10f, 10.0f,
				     0.25f);
	obs_property_float_set_suffix(p, " s");

	p = obs_properties_add_int(props, "keyint_sec", TEXT_KEYINT_SEC, 0, 20,
				   1);
	obs_property_int_set_suffix(p, " s");

	p = obs_properties_add_list(props, "profile", TEXT_PROFILE,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	if (type_data->codec_type == kCMVideoCodecType_H264) {
		obs_property_list_add_string(p, "baseline", "baseline");
		obs_property_list_add_string(p, "main", "main");
		obs_property_list_add_string(p, "high", "high");
#ifdef ENABLE_HEVC
	} else if (type_data->codec_type == kCMVideoCodecType_HEVC) {
		obs_property_list_add_string(p, "main", "main");
		obs_property_list_add_string(p, "main10", "main10");
		if (__builtin_available(macOS 12.3, *)) {
			obs_property_list_add_string(p, "main 4:2:2 10",
						     "main42210");
		}
#endif
	}

	obs_properties_add_bool(props, "bframes", TEXT_BFRAMES);

	return props;
}

static obs_properties_t *vt_properties_prores(void *unused, void *data)
{
	UNUSED_PARAMETER(unused);
	struct vt_encoder_type_data *type_data = data;

	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(props, "codec_type", TEXT_PRORES_CODEC,
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	uint32_t codec_availability_flags = 0;

	size_t size = 0;
	struct vt_prores_encoder_data *encoder_list = NULL;
	if (type_data->hardware_accelerated) {
		size = vt_prores_hardware_encoder_list.num;
		encoder_list = vt_prores_hardware_encoder_list.array;
	} else {
		size = vt_prores_software_encoder_list.num;
		encoder_list = vt_prores_software_encoder_list.array;
	}

	for (size_t i = 0; i < size; ++i) {

		switch (encoder_list[i].codec_type) {
		case kCMVideoCodecType_AppleProRes422Proxy:
			codec_availability_flags |= (1 << 0);
			break;
		case kCMVideoCodecType_AppleProRes422LT:
			codec_availability_flags |= (1 << 1);
			break;
		case kCMVideoCodecType_AppleProRes422:
			codec_availability_flags |= (1 << 2);
			break;
		case kCMVideoCodecType_AppleProRes422HQ:
			codec_availability_flags |= (1 << 3);
			break;
		}
	}

	if (codec_availability_flags & (1 << 0))
		obs_property_list_add_int(
			p, obs_module_text("ProRes422Proxy"),
			kCMVideoCodecType_AppleProRes422Proxy);
	if (codec_availability_flags & (1 << 1))
		obs_property_list_add_int(p, obs_module_text("ProRes422LT"),
					  kCMVideoCodecType_AppleProRes422LT);
	if (codec_availability_flags & (1 << 2))
		obs_property_list_add_int(p, obs_module_text("ProRes422"),
					  kCMVideoCodecType_AppleProRes422);
	if (codec_availability_flags & (1 << 3))
		obs_property_list_add_int(p, obs_module_text("ProRes422HQ"),
					  kCMVideoCodecType_AppleProRes422HQ);

	return props;
}

static void vt_defaults(obs_data_t *settings, void *data)
{
	struct vt_encoder_type_data *type_data = data;

	obs_data_set_default_string(settings, "rate_control", "ABR");
	if (__builtin_available(macOS 13.0, *))
		if (type_data->hardware_accelerated
#ifndef __aarch64__
		    && (os_get_emulation_status() == true)
#endif
		)
			obs_data_set_default_string(settings, "rate_control",
						    "CBR");
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "quality", 60);
	obs_data_set_default_bool(settings, "limit_bitrate", false);
	obs_data_set_default_int(settings, "max_bitrate", 2500);
	obs_data_set_default_double(settings, "max_bitrate_window", 1.5f);
	obs_data_set_default_int(settings, "keyint_sec", 0);
	obs_data_set_default_string(
		settings, "profile",
		type_data->codec_type == kCMVideoCodecType_H264 ? "high"
								: "main");
	obs_data_set_default_int(settings, "codec_type",
				 kCMVideoCodecType_AppleProRes422);
	obs_data_set_default_bool(settings, "bframes", true);
}

static void vt_free_type_data(void *data)
{
	struct vt_encoder_type_data *type_data = data;

	bfree((char *)type_data->disp_name);
	bfree((char *)type_data->id);
	bfree(type_data);
}

static inline void
vt_add_prores_encoder_data_to_list(CFDictionaryRef encoder_dict,
				   FourCharCode codec_type)
{
	struct vt_prores_encoder_data *encoder_data = NULL;

	CFBooleanRef hardware_accelerated = CFDictionaryGetValue(
		encoder_dict, kVTVideoEncoderList_IsHardwareAccelerated);
	if (hardware_accelerated == kCFBooleanTrue)
		encoder_data =
			da_push_back_new(vt_prores_hardware_encoder_list);
	else
		encoder_data =
			da_push_back_new(vt_prores_software_encoder_list);

	encoder_data->encoder_id = CFDictionaryGetValue(
		encoder_dict, kVTVideoEncoderList_EncoderID);

	encoder_data->codec_type = codec_type;
}

static CFComparisonResult
compare_encoder_list(const void *left_val, const void *right_val, void *unused)
{
	UNUSED_PARAMETER(unused);

	CFDictionaryRef left = (CFDictionaryRef)left_val;
	CFDictionaryRef right = (CFDictionaryRef)right_val;

	CFNumberRef left_codec_num =
		CFDictionaryGetValue(left, kVTVideoEncoderList_CodecType);
	CFNumberRef right_codec_num =
		CFDictionaryGetValue(right, kVTVideoEncoderList_CodecType);
	CFComparisonResult result =
		CFNumberCompare(left_codec_num, right_codec_num, NULL);

	if (result != kCFCompareEqualTo)
		return result;

	CFBooleanRef left_hardware_accel = CFDictionaryGetValue(
		left, kVTVideoEncoderList_IsHardwareAccelerated);
	CFBooleanRef right_hardware_accel = CFDictionaryGetValue(
		right, kVTVideoEncoderList_IsHardwareAccelerated);

	if (left_hardware_accel == right_hardware_accel)
		return kCFCompareEqualTo;
	else if (left_hardware_accel == kCFBooleanTrue)
		return kCFCompareGreaterThan;
	else
		return kCFCompareLessThan;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("mac-videotoolbox", "en-US")

bool obs_module_load(void)
{
	struct obs_encoder_info info = {
		.type = OBS_ENCODER_VIDEO,
		.get_name = vt_getname,
		.create = vt_create,
		.destroy = vt_destroy,
		.encode = vt_encode,
		.update = vt_update,
		.get_defaults2 = vt_defaults,
		.get_extra_data = vt_extra_data,
		.free_type_data = vt_free_type_data,
		.caps = OBS_ENCODER_CAP_DYN_BITRATE,
	};

	da_init(vt_prores_hardware_encoder_list);
	da_init(vt_prores_software_encoder_list);

	CFArrayRef encoder_list_const;
	VTCopyVideoEncoderList(NULL, &encoder_list_const);
	CFIndex size = CFArrayGetCount(encoder_list_const);

	CFMutableArrayRef encoder_list = CFArrayCreateMutableCopy(
		kCFAllocatorDefault, size, encoder_list_const);
	CFRelease(encoder_list_const);

	CFArraySortValues(encoder_list, CFRangeMake(0, size),
			  &compare_encoder_list, NULL);

	for (CFIndex i = 0; i < size; i++) {
		CFDictionaryRef encoder_dict =
			CFArrayGetValueAtIndex(encoder_list, i);

#define VT_DICTSTR(key, name)                                                 \
	CFStringRef name##_ref = CFDictionaryGetValue(encoder_dict, key);     \
	CFIndex name##_len =                                                  \
		CFStringGetMaximumSizeOfFileSystemRepresentation(name##_ref); \
	char *name = bzalloc(name##_len + 1);                                 \
	CFStringGetFileSystemRepresentation(name##_ref, name, name##_len);

		CMVideoCodecType codec_type = 0;
		{
			CFNumberRef codec_type_num = CFDictionaryGetValue(
				encoder_dict, kVTVideoEncoderList_CodecType);
			CFNumberGetValue(codec_type_num, kCFNumberSInt32Type,
					 &codec_type);
		}

		switch (codec_type) {
		case kCMVideoCodecType_H264:
			info.get_properties2 = vt_properties_h26x;
			info.codec = "h264";
			break;
#ifdef ENABLE_HEVC
		case kCMVideoCodecType_HEVC:
			info.get_properties2 = vt_properties_h26x;
			info.codec = "hevc";
			break;
#endif
			// 422 is used as a marker for all ProRes types,
			// since the type is stored as a profile
		case kCMVideoCodecType_AppleProRes422:
			info.get_properties2 = vt_properties_prores;
			info.codec = "prores";
			vt_add_prores_encoder_data_to_list(encoder_dict,
							   codec_type);
			break;

		case kCMVideoCodecType_AppleProRes422Proxy:
		case kCMVideoCodecType_AppleProRes422LT:
		case kCMVideoCodecType_AppleProRes422HQ:
			vt_add_prores_encoder_data_to_list(encoder_dict,
							   codec_type);
			continue;

		default:
			continue;
		}
		VT_DICTSTR(kVTVideoEncoderList_EncoderID, id);
		VT_DICTSTR(kVTVideoEncoderList_DisplayName, disp_name);

		CFBooleanRef hardware_ref = CFDictionaryGetValue(
			encoder_dict,
			kVTVideoEncoderList_IsHardwareAccelerated);

		bool hardware_accelerated =
			(hardware_ref) ? CFBooleanGetValue(hardware_ref)
				       : false;

		info.id = id;
		struct vt_encoder_type_data *type_data =
			bzalloc(sizeof(struct vt_encoder_type_data));
		type_data->disp_name = disp_name;
		type_data->id = id;
		type_data->codec_type = codec_type;
		type_data->hardware_accelerated = hardware_accelerated;
		info.type_data = type_data;

		obs_register_encoder(&info);
#undef VT_DICTSTR
	}

	CFRelease(encoder_list);

	VT_LOG(LOG_INFO, "Adding VideoToolbox encoders");

	return true;
}

void obs_module_unload(void)
{
	da_free(vt_prores_hardware_encoder_list);
	da_free(vt_prores_software_encoder_list);
}
