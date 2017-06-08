#include <obs-module.h>
#include <util/darray.h>
#include <util/platform.h>

#include "vaapi-encoder.h"
#include "vaapi-caps.h"

//#define DEBUG_H264
#ifdef DEBUG_H264
#include <stdio.h>
#endif

struct vaapi_enc
{
	obs_encoder_t *encoder;
	vaapi_display_t *display;
	uint32_t bitrate;
	bool use_bufsize;
	uint32_t buffer_size;
	uint32_t keyint_sec;
	uint32_t width;
	uint32_t height;
	uint32_t framerate_num;
	uint32_t framerate_den;
	vaapi_rc_t rc_type;
	uint32_t min_qp;
	uint32_t qp;
	uint32_t max_qp_delta;

	vaapi_profile_t profile;

	enum video_format format;
	vaapi_profile_caps_t *caps;
	vaapi_encoder_t *vaapi_encoder;

	vaapi_slice_type_t packet_slice_type;
	uint64_t packet_pts;
	DARRAY(uint8_t) packet;

#ifdef DEBUG_H264
	FILE *debug_file;
	bool header;
#endif

};

static void vaapi_enc_video_info(void *data,
		struct video_scale_info *info)
{
	struct vaapi_enc *enc = data;

	info->format = enc->format;
}

static const char *vaapi_enc_getname(void)
{
	return obs_module_text("VaapiH264Encoder");
}

static void update_params(struct vaapi_enc *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct video_scale_info info;

	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	vaapi_enc_video_info(enc, &info);

	vaapi_display_type_t device_type =
			(vaapi_display_type_t)obs_data_get_int(settings,
					"device_type");
	const char *device    = obs_data_get_string(settings, "device");
	uint32_t bitrate      = (uint32_t)obs_data_get_int(settings, "bitrate");
	uint32_t buffer_size  = (uint32_t)obs_data_get_int(settings,
			"buffer_size");
	uint32_t keyint_sec   = (uint32_t)obs_data_get_int(settings,
			"keyint_sec");
	uint32_t width        = (uint32_t)obs_encoder_get_width(enc->encoder);
	uint32_t height       = (uint32_t)obs_encoder_get_height(enc->encoder);
	bool use_bufsize      = obs_data_get_bool(settings, "use_bufsize");
	vaapi_rc_t rc_type    = (vaapi_rc_t)obs_data_get_int(settings,
			"rc_type");
	uint32_t min_qp       = (uint32_t)obs_data_get_int(settings, "min_qp");
	uint32_t qp           = (uint32_t)obs_data_get_int(settings, "qp");
	uint32_t max_qp_delta = (uint32_t)obs_data_get_int(settings,
			"max_qp_delta");
	vaapi_profile_t profile = (vaapi_profile_t)obs_data_get_int(settings,
			"profile");

	enc->display = vaapi_find_display(device_type, device);
	enc->bitrate = bitrate;
	enc->use_bufsize = use_bufsize;
	enc->buffer_size = buffer_size;
	enc->keyint_sec = keyint_sec;
	enc->width = width;
	enc->height = height;
	enc->framerate_num = voi->fps_num;
	enc->framerate_den = voi->fps_den;
	enc->rc_type = rc_type;
	enc->min_qp = min_qp;
	enc->qp = qp;
	enc->max_qp_delta = max_qp_delta;

	enc->profile = profile;
}

static bool vaapi_enc_update(void *data, obs_data_t *settings)
{
	struct vaapi_enc *enc = data;
	update_params(enc, settings);
	return true;
}

static bool vaapi_enc_extra_data(void *data, uint8_t **extra_data, size_t *size)
{
	struct vaapi_enc *enc = data;

	return vaapi_encoder_extra_data(enc->vaapi_encoder, extra_data, size);
}

static bool vaapi_enc_sei(void *data, uint8_t **sei, size_t *size)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(sei);
	UNUSED_PARAMETER(size);
	return false;
}

void coded_block(void *opaque, coded_block_entry_t *e)
{
	struct vaapi_enc *enc = opaque;
 	if (e->type == VAAPI_SLICE_TYPE_I) {
		size_t extra_data_size;
		uint8_t *extra_data;
		if (vaapi_encoder_extra_data(enc->vaapi_encoder, &extra_data,
				&extra_data_size)) {
			da_push_back_array(enc->packet, extra_data,
					extra_data_size);
		}
	}
	da_push_back_array(enc->packet, e->data.array, e->data.num);
	enc->packet_slice_type = e->type;
	enc->packet_pts = e->pts;

	#ifdef DEBUG_H264
	fwrite(enc->packet.array, sizeof(uint8_t), enc->packet.num,
			enc->debug_file);
	#endif

	da_free(e->data);
}

static bool vaapi_enc_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	struct vaapi_enc *enc = data;

	da_resize(enc->packet, 0);

	if (!vaapi_encoder_encode(enc->vaapi_encoder, frame)) {
		*received_packet = false;
		return false;
	}

	if (enc->packet.num == 0) {
		*received_packet = false;
		return true;
	}

	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = enc->packet_pts;
	packet->dts = enc->packet_pts;
	packet->data = enc->packet.array;
	packet->size = enc->packet.num;
	packet->keyframe = enc->packet_slice_type == VAAPI_SLICE_TYPE_I;
	*received_packet = true;
	return true;
}

static void vaapi_enc_destroy(void *data)
{
	struct vaapi_enc *enc = data;

	if (enc) {
		if (enc->vaapi_encoder != NULL)
			vaapi_encoder_destroy(enc->vaapi_encoder);
		da_free(enc->packet);

#ifdef DEBUG_H264
		fclose(enc->debug_file);
#endif

		bfree(enc);
	}
}

static void *vaapi_enc_create(obs_data_t *settings,
		obs_encoder_t *encoder)
{
	struct vaapi_enc *enc = bzalloc(sizeof(struct vaapi_enc));

	enc->encoder = encoder;

	update_params(enc, settings);
	vaapi_encoder_attribs_t attribs = {
		.display = enc->display,
		.profile = enc->profile,
		.width = enc->width,
		.height = enc->height,
		.bitrate = enc->bitrate,
		.rc_type = enc->rc_type,
		.min_qp = enc->min_qp,
		.qp = enc->qp,
		.max_qp_delta = enc->max_qp_delta,
		.framerate_num = enc->framerate_num,
		.framerate_den = enc->framerate_den,
		.keyint = enc->keyint_sec,
		.use_custom_cpb = enc->use_bufsize,
		.cpb_size = enc->buffer_size,
		.cpb_window_ms = 1500,
		.refpic_cnt = 3,
		.surface_cnt = 2,
		.coded_block_cb = coded_block,
		.coded_block_cb_opaque = enc
	};
	enc->format = VIDEO_FORMAT_NV12;

	enc->vaapi_encoder = vaapi_encoder_create(&attribs);

	if (enc->vaapi_encoder == NULL)
		goto fail;

#ifdef DEBUG_H264
	enc->debug_file = fopen("/home/jrb/Videos/out.h264", "wb");
#endif

	return enc;

fail:
	vaapi_enc_destroy(enc);

	return NULL;
}

#define TEXT_BITRATE      obs_module_text("Bitrate")
#define TEXT_CUSTOM_BUF   obs_module_text("CustomBufsize")
#define TEXT_BUF_SIZE     obs_module_text("BufferSize")
#define TEXT_RC_TYPE      obs_module_text("RCType")
#define TEXT_RC_CBR       obs_module_text("RCTypeCBR")
#define TEXT_RC_CQP       obs_module_text("RCTypeCQP")
#define TEXT_RC_VBR       obs_module_text("RCTypeVBR")
#define TEXT_RC_VBR_CON   obs_module_text("RCTypeVBRConstrained")
#define TEXT_QP           obs_module_text("QP")
#define TEXT_MIN_QP       obs_module_text("MinQP")
#define TEXT_MAX_QP_DELTA obs_module_text("MaxQPDelta")
#define TEXT_KEYINT_SEC   obs_module_text("KeyframeIntervalSec")
#define TEXT_PROFILE      obs_module_text("Profile")
#define TEXT_NONE         obs_module_text("None")
#define TEXT_DEVICE_TYPE  obs_module_text("DeviceType")
#define TEXT_X11_DEVICE   obs_module_text("X11Device")
#define TEXT_DRM_DEVICE   obs_module_text("DRMDevice")
#define TEXT_DEVICE       obs_module_text("Device")

static void set_visible(obs_properties_t *ppts, const char *name, bool visible)
{
	obs_property_t *p = obs_properties_get(ppts, name);
	obs_property_set_visible(p, visible);
}

static bool use_bufsize_modified(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	bool use_bufsize = obs_data_get_bool(settings, "use_bufsize");
	set_visible(ppts, "buffer_size", use_bufsize);

	return true;
}

static bool rc_type_modified(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	vaapi_rc_t rc_type = (vaapi_rc_t)obs_data_get_int(settings, "rc_type");

	set_visible(ppts, "bitrate", false);
	set_visible(ppts, "use_bufsize", false);
	set_visible(ppts, "buffer_size", false);
	set_visible(ppts, "min_qp", false);
	set_visible(ppts, "qp", false);
	set_visible(ppts, "max_qp_delta", false);

	if (rc_type == VAAPI_RC_CBR)
		set_visible(ppts, "bitrate", true);

	if (rc_type == VAAPI_RC_CBR ||
	    rc_type == VAAPI_RC_VBR ||
	    rc_type == VAAPI_RC_VBR_CONSTRAINED) {
		set_visible(ppts, "use_bufsize", true);
		use_bufsize_modified(ppts, NULL, settings);
	}

	if (rc_type == VAAPI_RC_CQP ||
	    rc_type == VAAPI_RC_VBR ||
	    rc_type == VAAPI_RC_VBR_CONSTRAINED) {
		set_visible(ppts, "qp", true);
	}

	if (rc_type == VAAPI_RC_VBR ||
	    rc_type == VAAPI_RC_VBR_CONSTRAINED) {
		set_visible(ppts, "min_qp", true);
		set_visible(ppts, "max_qp_delta", true);
	}

	return true;
}

static bool device_type_modified(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	vaapi_display_type_t device_type =
			(vaapi_display_type_t)obs_data_get_int(settings,
					"device_type");
	p = obs_properties_get(ppts, "device");
	obs_property_list_clear(p);
	for(size_t i = 0, cnt = vaapi_get_display_cnt(); i < cnt; i++) {
		vaapi_display_t *d = vaapi_get_display(i);
		if (vaapi_display_type(d) == device_type) {
			obs_property_list_add_string(p,
					vaapi_display_name(d),
					vaapi_display_path(d));
		}

	}
	return true;
}

static obs_properties_t *vaapi_enc_props(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	obs_property_t *list;
	obs_property_t *p;

	list = obs_properties_add_list(props, "device_type", TEXT_DEVICE_TYPE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, TEXT_X11_DEVICE, VAAPI_DISPLAY_X);
	obs_property_list_add_int(list, TEXT_DRM_DEVICE, VAAPI_DISPLAY_DRM);
	obs_property_set_modified_callback(list, device_type_modified);
	obs_properties_add_list(props, "device", TEXT_DEVICE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	list = obs_properties_add_list(props, "profile", TEXT_PROFILE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, TEXT_NONE, VAAPI_PROFILE_NONE);
	obs_property_list_add_int(list, "baseline", VAAPI_PROFILE_BASELINE);
	obs_property_list_add_int(list, "main", VAAPI_PROFILE_MAIN);
	obs_property_list_add_int(list, "high", VAAPI_PROFILE_HIGH);

	obs_properties_add_int(props, "keyint_sec", TEXT_KEYINT_SEC, 1, 20, 1);

	list = obs_properties_add_list(props, "rc_type", TEXT_RC_TYPE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(list, TEXT_RC_CBR, VAAPI_RC_CBR);
	obs_property_list_add_int(list, TEXT_RC_CQP, VAAPI_RC_CQP);
	obs_property_list_add_int(list, TEXT_RC_VBR, VAAPI_RC_VBR);
	obs_property_list_add_int(list, TEXT_RC_VBR_CON,
			VAAPI_RC_VBR_CONSTRAINED);

	obs_property_set_modified_callback(list, rc_type_modified);

	obs_properties_add_int(props, "bitrate", TEXT_BITRATE, 50, 10000000, 1);

	p = obs_properties_add_bool(props, "use_bufsize", TEXT_CUSTOM_BUF);
	obs_property_set_modified_callback(p, use_bufsize_modified);
	obs_properties_add_int(props, "buffer_size", TEXT_BUF_SIZE, 0,
			10000000, 1);

	obs_properties_add_int(props, "min_qp", TEXT_MIN_QP, 1, 51, 1);
	obs_properties_add_int(props, "qp", TEXT_QP, 1, 51, 1);
	obs_properties_add_int(props, "max_qp_delta",
			TEXT_MAX_QP_DELTA, 1, 10, 1);

	return props;
}

static void vaapi_enc_defaults(obs_data_t *settings)
{
	obs_data_set_default_int   (settings, "device_type",  VAAPI_DISPLAY_X);
	obs_data_set_default_string(settings, "device",       "");
	obs_data_set_default_int   (settings, "bitrate",      2500);
	obs_data_set_default_bool  (settings, "use_bufsize",  false);
	obs_data_set_default_int   (settings, "buffer_size",  2500);
	obs_data_set_default_int   (settings, "keyint_sec",   2);
	obs_data_set_default_int   (settings, "qp",           26);
	obs_data_set_default_int   (settings, "min_qp",       1);
	obs_data_set_default_int   (settings, "max_qp_delta", 4);
	obs_data_set_default_bool  (settings, "rc_type",      VAAPI_RC_CBR);
	obs_data_set_default_int   (settings, "profile",
			VAAPI_PROFILE_MAIN);
}

struct obs_encoder_info obs_vaapi_encoder = {
	.id             = "vaapi_h264",
	.type           = OBS_ENCODER_VIDEO,
	.codec          = "h264",
	.get_name       = vaapi_enc_getname,
	.create         = vaapi_enc_create,
	.destroy        = vaapi_enc_destroy,
	.encode         = vaapi_enc_encode,
	.update         = vaapi_enc_update,
	.get_properties = vaapi_enc_props,
	.get_defaults   = vaapi_enc_defaults,
	.get_extra_data = vaapi_enc_extra_data,
	.get_sei_data   = vaapi_enc_sei,
	.get_video_info = vaapi_enc_video_info
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("linux-vaapi", "en-US")

bool obs_module_load(void)
{
	if (!vaapi_initialize()) {
		VA_LOG(LOG_WARNING, "no VA-API supported devices found with "
				"encoding support");
		return false;
	}

	obs_register_encoder(&obs_vaapi_encoder);
	return true;
}

void obs_module_unload(void)
{
	vaapi_shutdown();
}
