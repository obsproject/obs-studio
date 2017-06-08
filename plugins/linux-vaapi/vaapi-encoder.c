#include <obs-module.h>
#include <util/darray.h>
#include <util/platform.h>

#include <va/va.h>
#include <va/va_x11.h>
#include <va/va_enc_h264.h>
#include <X11/Xlib.h>

#include "bitstream.h"
#include "surface-queue.h"
#include "vaapi-caps.h"
#include "vaapi-common.h"
#include "vaapi-internal.h"
#include "vaapi-encoder.h"

// SPS Constants
enum {
	SPS_PROFILE_IDC_BASELINE     = 66,
	SPS_PROFILE_IDC_MAIN         = 77,
	SPS_PROFILE_IDC_HIGH         = 100,
};

// CPB Constants
enum {
	HRD_DEFAULT_CPB_WINDOW_SIZE  = 1500
};

// HRD Constants (E.2.2)
enum {
	HRD_BITRATE_SCALE            = 6, // E-37
	HRD_CPB_SIZE_SCALE           = 4, // E-38
	HRD_INIT_CPB_REM_DELAY_LEN   = 24,
	HRD_DPB_OUTPUT_DELAY_LEN     = 24,
	HRD_TIME_OFFSET_LEN          = 24
};

enum {
	SPS_CHROMA_FORMAT_MONOCHROME = 0,
	SPS_CHROMA_FORMAT_420        = 1,
	SPS_CHROMA_FORMAT_422        = 2,
	SPS_CHROMA_FORMAT_444        = 3,
};

enum nal_unit_type
{
    NAL_UNKNOWN     = 0,
    NAL_SLICE       = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,
    NAL_PPS         = 8,
    NAL_AUD         = 9,
    NAL_FILLER      = 12,
    /* ref_idc == 0 for 6,9,10,11,12 */
};

enum nal_priority
{
    NAL_PRIORITY_DISPOSABLE = 0,
    NAL_PRIORITY_LOW        = 1,
    NAL_PRIORITY_HIGH       = 2,
    NAL_PRIORITY_HIGHEST    = 3,
};

enum sei_type
{
  SEI_BUF_PERIOD = 0,
  SEI_PIC_TIMING = 1
};

struct vaapi_encoder
{
	vaapi_display_t *vdisplay;
	VADisplay display;
	VAConfigID config;
	VAContextID context;
	DARRAY(VASurfaceID) refpics;

	surface_queue_t *surfq;

	vaapi_profile_caps_t *caps;

	uint32_t width;
	uint32_t height;
	uint32_t width_in_mbs;
	uint32_t height_in_mbs;

	uint32_t keyint;
	uint32_t framerate_num;
	uint32_t framerate_den;
	vaapi_format_t format;

	uint32_t intra_period;

	struct {
		vaapi_rc_t type;
		uint32_t bitrate;
		uint32_t bitrate_bits;
		uint32_t qp;
		uint32_t min_qp;
		uint32_t max_qp_delta;
		bool use_custom_cpb;
		uint32_t cpb_window_ms;
		uint32_t cpb_size;
		uint32_t cpb_size_bits;
	} rc;

	VAEncSequenceParameterBufferH264 sps;
	VAEncPictureParameterBufferH264 pps;
	VAEncSliceParameterBufferH264 slice;

	uint64_t frame_cnt;
	uint64_t gop_idx;
	uint32_t output_buf_size;

	uint32_t surface_cnt;
	void *coded_block_cb_opaque;
	vaapi_coded_block_cb coded_block_cb;
	DARRAY(uint8_t) extra_data;
};

typedef struct darray buffer_list_t;

void vaapi_encoder_destroy(vaapi_encoder_t *enc)
{
	if (enc != NULL) {
		vaapi_display_lock(enc->vdisplay);

		vaDestroySurfaces(enc->display, enc->refpics.array,
				enc->refpics.num);

		da_free(enc->refpics);
		da_free(enc->extra_data);

		surface_queue_destroy(enc->surfq);

		vaDestroyConfig(enc->display, enc->config);
		vaDestroyContext(enc->display, enc->context);

		vaapi_display_unlock(enc->vdisplay);

		bfree(enc);
	}
}

static uint32_t vaapi_rc_to_va_rc(vaapi_rc_t rc)
{
	switch(rc)
	{
		case VAAPI_RC_CBR: return VA_RC_CBR;
		case VAAPI_RC_CQP: return VA_RC_CQP;
		case VAAPI_RC_VBR: return VA_RC_VBR;
		case VAAPI_RC_VBR_CONSTRAINED: return VA_RC_VBR_CONSTRAINED;
		default: return VA_RC_NONE;
	}
}

static bool prepare_rate_control(vaapi_encoder_t *enc, VAConfigAttrib *a)
{
	uint32_t va_rc = vaapi_rc_to_va_rc(enc->rc.type);
	if ((a->value & va_rc) == 0) {
		VA_LOG(LOG_ERROR, "hardware doesn't support specified '%s'",
				va_rc_to_str(enc->rc.type));
		return false;
	}

	a->value &= va_rc;

	return true;
}
static bool prepare_encoder_attributes(vaapi_encoder_t *enc,
		struct darray *attribs)
{
	for(size_t i = 0; i < enc->caps->attribs_cnt; i++) {
		VAConfigAttrib attr = enc->caps->attribs[i];
		switch (enc->caps->attribs[i].type)
		{
		case VAConfigAttribRateControl:
			if (!prepare_rate_control(enc, &attr))
				return false;
			break;
		default: // todo: add config to str; ignore for now
			break;
		}

		darray_push_back(sizeof(VAConfigAttrib), attribs, &attr);
	}

	return true;
}

static bool initialize_encoder(vaapi_encoder_t *enc)
{
	VAStatus status;

	DARRAY(VAConfigAttrib) encoder_attribs = {0};
	prepare_encoder_attributes(enc, &encoder_attribs.da);

	CHECK_STATUS_FALSE(vaCreateConfig(enc->display, enc->caps->def.va,
			enc->caps->entrypoint, encoder_attribs.array,
			encoder_attribs.num, &enc->config));

	CHECK_STATUS_FAIL(vaCreateContext(enc->display, enc->config,
			enc->width, enc->height, VA_PROGRESSIVE, NULL, 0,
			&enc->context));

	CHECK_STATUS_FAILN(vaCreateSurfaces(enc->display, VA_RT_FORMAT_YUV420,
			enc->width, enc->height, enc->refpics.array,
			enc->refpics.num, NULL, 0), 1);

	da_free(encoder_attribs);

	return true;

fail1:
	vaDestroyContext(enc->display, enc->context);

fail:
	vaDestroyConfig(enc->display, enc->config);
	da_free(encoder_attribs);

	return false;
}

#define HRD_SCALE(scale) (~((1U << scale) - 1))

bool vaapi_encoder_set_cpb(vaapi_encoder_t *enc, uint32_t cpb_window_ms,
		uint32_t cpb_size)
{
	enc->rc.cpb_window_ms = cpb_window_ms;
	enc->rc.cpb_size = cpb_size;
	enc->rc.cpb_size_bits = cpb_size * cpb_window_ms;
	enc->rc.cpb_size_bits &= HRD_SCALE(HRD_CPB_SIZE_SCALE);

	return true;
}

bool vaapi_encoder_set_bitrate(vaapi_encoder_t *enc, uint32_t bitrate)
{
	enc->rc.bitrate = bitrate;
	enc->rc.bitrate_bits = bitrate * 1000;
	enc->rc.bitrate_bits &= HRD_SCALE(HRD_BITRATE_SCALE);

	return true;
}

static void init_sps(vaapi_encoder_t *enc)
{
	memset(&enc->sps, 0, sizeof(VAEncSequenceParameterBufferH264));

	int frame_cropping_flag = 0;
	int frame_crop_bottom_offset = 0;
	int frame_crop_right_offset = 0;

#define SPS enc->sps
	SPS.level_idc = 52;
	SPS.intra_period = enc->intra_period;
	SPS.bits_per_second = enc->rc.bitrate_bits;
	SPS.max_num_ref_frames = 4;
	SPS.picture_width_in_mbs = enc->width_in_mbs;
	SPS.picture_height_in_mbs = enc->height_in_mbs;
	SPS.seq_fields.bits.frame_mbs_only_flag = 1;
	SPS.seq_fields.bits.chroma_format_idc = SPS_CHROMA_FORMAT_420;

	SPS.time_scale = enc->framerate_num * 2;
	SPS.num_units_in_tick = enc->framerate_den;
	SPS.vui_fields.bits.timing_info_present_flag = true;
	SPS.vui_fields.bits.bitstream_restriction_flag = true;

	if (enc->height_in_mbs * 16 - enc->height > 0) {
		frame_cropping_flag = 1;
		frame_crop_bottom_offset =
				(enc->height_in_mbs * 16 - enc->height) / 2;
	}

	if (enc->width_in_mbs * 16 - enc->width > 0) {
		frame_cropping_flag = 1;
		frame_crop_right_offset =
				(enc->width_in_mbs * 16 - enc->width) / 2;
	}

	SPS.frame_cropping_flag = frame_cropping_flag;
	SPS.frame_crop_bottom_offset = frame_crop_bottom_offset;
	SPS.frame_crop_right_offset = frame_crop_right_offset;

	SPS.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = 2;

#undef SPS
}

static void init_pps(vaapi_encoder_t *enc)
{
#define PPS enc->pps
	PPS.pic_init_qp = enc->rc.qp;

	PPS.pic_fields.bits.entropy_coding_mode_flag =
			enc->caps->def.vaapi == VAAPI_PROFILE_MAIN ||
			enc->caps->def.vaapi == VAAPI_PROFILE_HIGH;
	PPS.pic_fields.bits.deblocking_filter_control_present_flag = 1;
#undef PPS
}

bool pack_pps(vaapi_encoder_t *enc, bitstream_t *bs)
{
	bs_begin_nalu(bs, NAL_PPS, NAL_PRIORITY_HIGHEST);

#define PIC enc->pps
#define APPEND_PFB(c, x) bs_append_bits(bs, c, PIC.pic_fields.bits.x)

	// pic_parameter_set_id
	bs_append_ue(bs, PIC.pic_parameter_set_id);
	// seq_parameter_set_id
	bs_append_ue(bs, PIC.seq_parameter_set_id);

	APPEND_PFB(1, entropy_coding_mode_flag);

	// pic_order_present_flag
	bs_append_bool(bs, false);

	// num_slice_groups_minus1
	bs_append_ue(bs, 0);

	// num_ref_idx_l0_active_minus1
	bs_append_ue(bs, PIC.num_ref_idx_l0_active_minus1);
	// num_ref_idx_l1_active_minus1
	bs_append_ue(bs, PIC.num_ref_idx_l1_active_minus1);

	// weighted_pred_flag
	APPEND_PFB(1, weighted_pred_flag);
	// weighted_bipred_idc
	APPEND_PFB(2, weighted_bipred_idc);

	// pic_init_qp_minus26
	bs_append_se(bs, PIC.pic_init_qp - 26);
	// pic_init_qs_minus26
	bs_append_se(bs, 0);
	// chroma_qp_index_offset
	bs_append_se(bs, 0);

	APPEND_PFB(1, deblocking_filter_control_present_flag);

	// constrained_intra_pred_flag,
	bs_append_bits(bs, 1, 0);
	// redundant_pic_cnt_present_flag
	bs_append_bits(bs, 1, 0);

	if (enc->caps->def.vaapi == VAAPI_PROFILE_HIGH) {
		APPEND_PFB(1, transform_8x8_mode_flag);
		// pic_scaling_matrix_present_flag
		bs_append_bool(bs, false);
		bs_append_se(bs, PIC.second_chroma_qp_index_offset);
	}

	bs_end_nalu(bs);

	return true;

#undef APPEND_PFB
#undef PIC
}

bool pack_sps(vaapi_encoder_t *enc, bitstream_t *bs)
{
	int profile_idc;
	bool constraint_set0_flag = false;
	bool constraint_set1_flag = false;
	bool constraint_set2_flag = false;
	bool constraint_set3_flag = false;
	bool constraint_set4_flag = false;
	bool constraint_set5_flag = false;

	switch(enc->caps->def.va) {
	case VAProfileH264ConstrainedBaseline:
	case VAProfileH264Baseline:
		profile_idc = SPS_PROFILE_IDC_BASELINE;
		if (enc->caps->def.va == VAProfileH264ConstrainedBaseline)
			constraint_set0_flag = 1;
		break;
	case VAProfileH264Main:
		profile_idc = SPS_PROFILE_IDC_MAIN;
		constraint_set1_flag = 1;
		break;
	case VAProfileH264High:
		profile_idc = SPS_PROFILE_IDC_HIGH; break;
	default:
		VA_LOG(LOG_ERROR, "failed creating sps due to invalid profile");
		goto fail;
	}

#define SPS enc->sps

	bs_begin_nalu(bs, NAL_SPS, NAL_PRIORITY_HIGHEST);
	bs_append_bits(bs, 8, profile_idc);
	bs_append_bits(bs, 1, constraint_set0_flag);
	bs_append_bits(bs, 1, constraint_set1_flag);
	bs_append_bits(bs, 1, constraint_set2_flag);
	bs_append_bits(bs, 1, constraint_set3_flag);
	bs_append_bits(bs, 1, constraint_set4_flag);
	bs_append_bits(bs, 1, constraint_set5_flag);
	bs_append_bits(bs, 2, 0); // reserved 2 bits
	bs_append_bits(bs, 8, SPS.level_idc);
	bs_append_ue(bs, SPS.seq_parameter_set_id);

	if (profile_idc == SPS_PROFILE_IDC_HIGH) {
#define SPS_SEQ enc->sps.seq_fields.bits
		bs_append_ue(bs, SPS_SEQ.chroma_format_idc);
		if (SPS_SEQ.chroma_format_idc == SPS_CHROMA_FORMAT_444)
			// separate_colour_plane_flag
			bs_append_bool(bs, false);
		bs_append_ue(bs, SPS.bit_depth_luma_minus8);
		bs_append_ue(bs, SPS.bit_depth_chroma_minus8);
		// qpprime_y_zero_transform_bypass_flag
		bs_append_bool(bs, false);
		// scaling matrix not current supported
		assert(SPS_SEQ.seq_scaling_matrix_present_flag == 0);
		bs_append_bits(bs, 1, SPS_SEQ.seq_scaling_matrix_present_flag);
#undef SPS_SEQ
	}

#define APPEND_SFB(x) bs_append_ue(bs, SPS.seq_fields.bits.x);
	APPEND_SFB(log2_max_frame_num_minus4);
	APPEND_SFB(pic_order_cnt_type);
	APPEND_SFB(log2_max_pic_order_cnt_lsb_minus4);
#undef APPEND_SFB

	bs_append_ue(bs, SPS.max_num_ref_frames);

	bs_append_bits(bs, 1, 0); // gaps_in_frame_num_value_allowed_flag

	// pic_width_in_mbs_minus1
	bs_append_ue(bs, SPS.picture_width_in_mbs - 1);
	// pic_height_in_map_units_minus1
	bs_append_ue(bs, SPS.picture_height_in_mbs - 1);

#define APPEND_SFB_BIT(x) bs_append_bits(bs, 1, SPS.seq_fields.bits.x);
	APPEND_SFB_BIT(frame_mbs_only_flag);
	APPEND_SFB_BIT(direct_8x8_inference_flag);
#undef APPEND_SFB_BIT

	bs_append_bits(bs, 1, SPS.frame_cropping_flag);
	if (SPS.frame_cropping_flag) {
		bs_append_ue(bs, SPS.frame_crop_left_offset);
		bs_append_ue(bs, SPS.frame_crop_right_offset);
		bs_append_ue(bs, SPS.frame_crop_top_offset);
		bs_append_ue(bs, SPS.frame_crop_bottom_offset);
	}

	// vui_parameters_present_flag
	bs_append_bits(bs, 1, 1);

	// aspect_ratio_info_present_flag
	bs_append_bits(bs, 1, 0);
	// overscan_info_present_flag
	bs_append_bits(bs, 1, 0);

	// video_signal_type_present_flag
	bs_append_bits(bs, 1, 0);
	// chroma_loc_info_present_flag
	bs_append_bits(bs, 1, 0);

	// timing_info_present_flag
	bs_append_bits(bs, 1, SPS.vui_fields.bits.timing_info_present_flag);
	{
		bs_append_bits(bs, 32, SPS.num_units_in_tick);
		bs_append_bits(bs, 32, SPS.time_scale);

		// fixed_frame_rate_flag
		bs_append_bool(bs, true);
	}

	bool nal_hrd_parameters_present_flag = SPS.bits_per_second > 0;

	// nal_hrd_parameters_present_flag
	bs_append_bool(bs, nal_hrd_parameters_present_flag);
	if (nal_hrd_parameters_present_flag) {

		int cpb_cnt_minus1 = 0;
		bs_append_ue(bs, cpb_cnt_minus1);
		// bit_rate_scale
		bs_append_bits(bs, 4, 0 /*default*/);
		// cpb_size_scale
		bs_append_bits(bs, 4, 0 /*default*/);
		for(int i = 0; i <= cpb_cnt_minus1; i++) {
			uint32_t bit_rate_scale = enc->rc.bitrate_bits;
			bit_rate_scale >>= HRD_BITRATE_SCALE;
			bs_append_ue(bs, bit_rate_scale - 1);
			uint32_t cpb_size_scale = enc->rc.cpb_size_bits;
			cpb_size_scale >>= HRD_CPB_SIZE_SCALE;
			bs_append_ue(bs, cpb_size_scale - 1);
			bs_append_bool(bs, enc->rc.type == VAAPI_RC_CBR);
		}
		// initial_cpb_removal_delay_length_minus1
		bs_append_bits(bs, 5, 23);
		// cpb_removal_delay_length_minus1
		bs_append_bits(bs, 5, 23);
		// dpb_output_delay_length_minus1
		bs_append_bits(bs, 5, 23);
		// time_offset_length
		bs_append_bits(bs, 5, 23);
	}

	bool vcl_hrd_parameters_present_flag = false;

	// vcl_hrd_parameters_present_flag
	bs_append_bool(bs, vcl_hrd_parameters_present_flag);

	if (nal_hrd_parameters_present_flag ||
	    vcl_hrd_parameters_present_flag) {
		// low_delay_hrd_flag
		bs_append_bool(bs, false);
	}

	// pic_struct_present_flag
	bs_append_bool(bs, false);

	// bitstream_restriction_flag
	bs_append_bits(bs, 1, SPS.vui_fields.bits.bitstream_restriction_flag);

	if (SPS.vui_fields.bits.bitstream_restriction_flag == 1) {
		// motion_vectors_over_pic_boundaries_flag
		bs_append_bool(bs, false);
		// max_bytes_per_pic_denom
		bs_append_ue(bs, 2);
		// max_bits_per_mb_denom
		bs_append_ue(bs, 1);
		// log2_max_mv_length_horizontal
		bs_append_ue(bs, 16);
		// log2_max_mv_length_vertical
		bs_append_ue(bs, 16);
		// disable B slices
		// max_num_reorder_frames
		bs_append_ue(bs, 0);
		// max_num_ref_frame
		bs_append_ue(bs, SPS.max_num_ref_frames);
	}

	bs_end_nalu(bs);

	return true;

fail:
	return false;

#undef SPS
}

static bool pack_sei_buf_period(vaapi_encoder_t *enc, bitstream_t *bs)
{
	int initial_cpb_removal_delay =
		((enc->rc.cpb_window_ms / 2) * 90000) / 1000;
	int initial_cpb_removal_delay_offset = 0;
	int sequence_parameter_set_id = 0;

	bs_append_ue(bs, sequence_parameter_set_id);
	bs_append_bits(bs, HRD_INIT_CPB_REM_DELAY_LEN,
			initial_cpb_removal_delay);
	bs_append_bits(bs, 24, initial_cpb_removal_delay_offset);

	bs_append_trailing_bits(bs);

	return true;
}

static bool pack_sei_pic_timing(vaapi_encoder_t *enc, bitstream_t *bs)
{
	int cpb_removal_delay = enc->gop_idx * 2 + 2;
	int dpb_removal_delay = 0;
	int pic_struct = 0;
	bool clock_timestamp_flag = false;
	bool pic_struct_present_flag = true;

	bs_append_bits(bs, HRD_INIT_CPB_REM_DELAY_LEN, cpb_removal_delay);
	bs_append_bits(bs, HRD_DPB_OUTPUT_DELAY_LEN, dpb_removal_delay);

	if (pic_struct_present_flag) {
		bs_append_bits(bs, 4, pic_struct);
		bs_append_bool(bs, clock_timestamp_flag);
	}

	bs_append_trailing_bits(bs);

	return true;
}

static bool pack_sei(vaapi_encoder_t *enc, bitstream_t *bs)
{
	bitstream_t *sei_bs = bs_create();

	bs_begin_nalu(bs, NAL_SEI, NAL_PRIORITY_DISPOSABLE);

	bs_reset(sei_bs);
	pack_sei_buf_period(enc, sei_bs);
	bs_append_bits(bs, 8, SEI_BUF_PERIOD);
	bs_append_bits(bs, 8, bs_size(sei_bs));
	bs_append_bs(bs, sei_bs);

	bs_reset(sei_bs);
	pack_sei_pic_timing(enc, sei_bs);
	bs_append_bits(bs, 8, SEI_PIC_TIMING);
	bs_append_bits(bs, 8, bs_size(sei_bs));
	bs_append_bs(bs, sei_bs);

	bs_end_nalu(bs);

	bs_free(sei_bs);

	return true;
}

static bool create_buffer(vaapi_encoder_t *enc, VABufferType type,
		unsigned int size, unsigned int num_elements, void *data,
		struct darray *list)
{
	VABufferID buf;
	VAStatus status;

	CHECK_STATUS_FALSE(vaCreateBuffer(enc->display, enc->context,
			type, size, num_elements, data, &buf));
	if (buf == VA_INVALID_ID) {
		VA_LOG(LOG_ERROR, "failed to create buffer");
		return false;
	}

	darray_push_back(sizeof(VABufferID), list, &buf);

	return true;
}

static VABufferID get_last_buffer(buffer_list_t *list)
{
	if (list->num > 0)
		return *((VABufferID *)darray_item(sizeof(VABufferID), list,
				list->num - 1));
	return VA_INVALID_ID;
}

static void destroy_last_buffer(vaapi_encoder_t *enc, buffer_list_t *list)
{
	if (list->num > 0) {
		VABufferID *buf = darray_item(sizeof(VABufferID), list,
				list->num - 1);
		if (*buf != VA_INVALID_ID) {
			vaDestroyBuffer(enc->display, *buf);
		}
		darray_pop_back(sizeof(VABufferID), list);
	}
}

static void destroy_buffers(vaapi_encoder_t *enc, buffer_list_t *list)
{
	for(size_t i = 0; i < list->num; i++) {
		VABufferID *buf = darray_item(sizeof(VABufferID), list, i);
		if (*buf != VA_INVALID_ID) {
			vaDestroyBuffer(enc->display, *buf);
		}
	}
	darray_resize(sizeof(VABufferID), list, 0);
}

static bool create_seq_buffer(vaapi_encoder_t *enc, buffer_list_t *list)
{
	return create_buffer(enc, VAEncSequenceParameterBufferType,
			sizeof(enc->sps), 1, &enc->sps, list);
}

static bool create_slice_buffer(vaapi_encoder_t *enc, buffer_list_t *list,
		vaapi_slice_type_t slice_type)
{
	memset(&enc->slice, 0, sizeof(enc->slice));

	enc->slice.num_macroblocks = enc->width_in_mbs * enc->height_in_mbs;
	enc->slice.slice_type = slice_type;

	enc->slice.slice_alpha_c0_offset_div2 = 2;
	enc->slice.slice_beta_offset_div2 = 2;

	enc->slice.slice_qp_delta = enc->rc.max_qp_delta;

	if (!create_buffer(enc, VAEncSliceParameterBufferType,
			sizeof(enc->slice), 1, &enc->slice, list)) {
		VA_LOG(LOG_ERROR, "failed to create "
				"VAEncSliceParameterBufferType buffer");
		return false;
	}

	return true;
}

static bool create_misc_buffer(vaapi_encoder_t *enc,
		VAEncMiscParameterType type, size_t size, void *data,
		buffer_list_t *list)
{
	VAStatus status;
	VABufferID buffer;
	void *buffer_data;

	if (!create_buffer(enc, VAEncMiscParameterBufferType,
			sizeof(VAEncMiscParameterBufferType) + size,
			1, data, list))
		return false;

	buffer = get_last_buffer(list);

	CHECK_STATUS_FAIL(vaMapBuffer(enc->display, buffer, &buffer_data));

	VAEncMiscParameterBuffer* misc_param =
			(VAEncMiscParameterBuffer*)buffer_data;

	misc_param->type = type;
	memcpy(misc_param->data, data, size);

	CHECK_STATUS_FAIL(vaUnmapBuffer(enc->display, buffer));

	return true;

fail:
	destroy_last_buffer(enc, list);

	return false;
}

static bool create_misc_rc_buffer(vaapi_encoder_t *enc,
		buffer_list_t *list)
{
	VAEncMiscParameterRateControl rc = {0};

	rc.bits_per_second = enc->rc.bitrate_bits;
	rc.target_percentage = 90;
	rc.window_size = enc->rc.cpb_window_ms;
	rc.initial_qp = enc->rc.qp;
	rc.min_qp = enc->rc.min_qp;

	rc.rc_flags.bits.disable_frame_skip = false;

	if (!create_misc_buffer(enc, VAEncMiscParameterTypeRateControl,
			sizeof(rc), &rc, list)) {
		VA_LOG(LOG_ERROR, "failed to create "
				"VAEncMiscParameterBufferType RC buffer");
		return false;
	}

	return true;
}

static bool create_misc_hdr_buffer(vaapi_encoder_t *enc,
		buffer_list_t *list)
{
	VAEncMiscParameterHRD hrd = {0};

	hrd.initial_buffer_fullness = enc->rc.cpb_size_bits / 2;
	hrd.buffer_size = enc->rc.cpb_size_bits;

	if (!create_misc_buffer(enc, VAEncMiscParameterTypeHRD,
			sizeof(hrd), &hrd, list)) {
		VA_LOG(LOG_ERROR, "failed to create "
				"VAEncMiscParameterBufferType HRD buffer");
		return false;
	}

	return true;
}

static bool create_pic_buffer(vaapi_encoder_t *enc, buffer_list_t *list,
		VABufferID output_buf)
{
	VASurfaceID curr_pic, pic0;

#define PPS enc->pps

	curr_pic = enc->refpics.array[enc->gop_idx % 2];
	pic0 = enc->refpics.array[(enc->gop_idx + 1) % 2];

	PPS.CurrPic.picture_id = curr_pic;
	PPS.CurrPic.frame_idx = enc->gop_idx;
	PPS.CurrPic.flags = 0;

	PPS.CurrPic.TopFieldOrderCnt = enc->gop_idx * 2;
	PPS.CurrPic.BottomFieldOrderCnt = PPS.CurrPic.TopFieldOrderCnt;

#define REF_FRAME_SIZE \
	(sizeof(PPS.ReferenceFrames) / sizeof(PPS.ReferenceFrames[0]))

	PPS.ReferenceFrames[0].picture_id = pic0;
	PPS.ReferenceFrames[1].picture_id = enc->refpics.array[2];
	for(size_t i = 2; i < REF_FRAME_SIZE; i++)
		PPS.ReferenceFrames[i].picture_id = VA_INVALID_ID;
#undef REF_FRAME_SIZE

	PPS.coded_buf = output_buf;
	PPS.frame_num = enc->gop_idx;
	PPS.pic_init_qp = enc->rc.qp;

	PPS.pic_fields.bits.idr_pic_flag = enc->gop_idx == 0;
	PPS.pic_fields.bits.reference_pic_flag = 1;

	if (!create_buffer(enc, VAEncPictureParameterBufferType,
				sizeof(VAEncPictureParameterBufferH264), 1,
				&PPS, list)) {
		VA_LOG(LOG_ERROR, "failed to create "
				"VAEncPictureParameterBufferH264 buffer");
		return false;
	}

	return true;
}

static bool create_packed_header_buffers(vaapi_encoder_t *enc,
		buffer_list_t *list, VAEncPackedHeaderType type,
		bitstream_t *bs)
{
	VAEncPackedHeaderParameterBuffer header;

	header.type = type;
	header.bit_length = bs_size(bs) * 8;
	header.has_emulation_bytes = 0;

	if (!create_buffer(enc,
			VAEncPackedHeaderParameterBufferType,
			sizeof(header), 1, &header, list)) {
		VA_LOG(LOG_ERROR, "failed to create "
				"VAEncPackedHeaderParameterBufferType buffer");
		goto fail;
	}
	if (!create_buffer(enc, VAEncPackedHeaderDataBufferType,
			bs_size(bs), 1, bs_data(bs), list)) {
		VA_LOG(LOG_ERROR, "failed to create "
				"VAEncPackedHeaderDataBufferType buffer");
		goto fail1;
	}

	return true;

fail1:
	destroy_last_buffer(enc, list);

fail:
	return false;
}

static VABufferID create_output_buffer(vaapi_encoder_t *enc)
{
	VABufferID output_buf;
	VAStatus status;

	CHECK_STATUS_FAIL(vaCreateBuffer(enc->display, enc->context,
				VAEncCodedBufferType, enc->output_buf_size,
				1, NULL, &output_buf));
	return output_buf;

fail:
	return VA_INVALID_ID;
}

static void encode_nalu_to_extra_data(vaapi_encoder_t *enc, bitstream_t *bs)
{
	size_t size = bs_size(bs);
	uint8_t *data = bs_data(bs);
	int zero_cnt = 0;
	// add header manually
	da_push_back_array(enc->extra_data, data, 5);
	// add EP to rbsp
	for(uint8_t *d = (data + 5); d < (data + size - 1); d++) {
		if (zero_cnt == 2 && *d <= 0x03) {
			static uint8_t EP_3 = 0x03;
			da_push_back(enc->extra_data, &EP_3);
			zero_cnt = 0;
		}

		if (*d == 0x00)
			zero_cnt++;
		else
			zero_cnt = 0;

		da_push_back(enc->extra_data, d);
	}
	da_push_back(enc->extra_data, (data + size - 1));
}


static bool create_sei_buffer(vaapi_encoder_t *enc,
		buffer_list_t *list)
{
	bitstream_t *bs = bs_create();

	pack_sei(enc, bs);
	create_packed_header_buffers(enc, list, VAEncPackedHeaderH264_SEI, bs);

	bs_free(bs);

	return true;
}

static bool create_packed_sps_pps_buffers(vaapi_encoder_t *enc,
		buffer_list_t *list)
{
	bitstream_t *bs = bs_create();

	pack_sps(enc, bs);
	create_packed_header_buffers(enc, list, VAEncPackedHeaderSequence,
			bs);
	encode_nalu_to_extra_data(enc, bs);

	bs_reset(bs);

	pack_pps(enc, bs);
	create_packed_header_buffers(enc, list, VAEncPackedHeaderPicture,
			bs);
	encode_nalu_to_extra_data(enc, bs);

	bs_free(bs);

	return true;
}

bool vaapi_encoder_extra_data(vaapi_encoder_t *enc,
		uint8_t **extra_data, size_t *extra_data_size)
{
	if (enc->extra_data.num == 0)
		return false;

	*extra_data = enc->extra_data.array;
	*extra_data_size = enc->extra_data.num;

	return true;
}

static bool render_picture(vaapi_encoder_t *enc, buffer_list_t *list,
		VASurfaceID input)
{
	VAStatus status;

	CHECK_STATUS_FALSE(vaBeginPicture(enc->display, enc->context, input));
	CHECK_STATUS_FALSE(vaRenderPicture(enc->display, enc->context,
			(VABufferID *)list->array, list->num));
	CHECK_STATUS_FALSE(vaEndPicture(enc->display, enc->context));
	CHECK_STATUS_FALSE(vaSyncSurface(enc->display, input));

	return true;
}

bool encode_surface(vaapi_encoder_t *enc, VASurfaceID input)
{
	DARRAY(VABufferID) buffers = {0};
	VABufferID output_buffer;
	vaapi_slice_type_t slice_type;

	// todo: implement b frame handling
	// for now, every I frame is an IDR
	if (enc->gop_idx == 0)
		slice_type = VAAPI_SLICE_TYPE_I;
	else
		slice_type = VAAPI_SLICE_TYPE_P;

	// todo: pool buffers
	if (!create_seq_buffer(enc, &buffers.da))
		goto fail;
	if (!create_misc_hdr_buffer(enc, &buffers.da))
		goto fail;
	if ((enc->rc.type == VAAPI_RC_CBR ||
	     enc->rc.type == VAAPI_RC_VBR) &&
	    !create_sei_buffer(enc, &buffers.da) &&
	    !create_misc_rc_buffer(enc, &buffers.da))
		goto fail;
	if (!create_slice_buffer(enc, &buffers.da, slice_type))
		goto fail;

	if ((enc->frame_cnt % enc->intra_period) == 0)
		if (!create_packed_sps_pps_buffers(enc, &buffers.da))
			goto fail;

	// can we reuse output buffers?
	output_buffer = create_output_buffer(enc);
	if (!create_pic_buffer(enc, &buffers.da, output_buffer))
		goto fail;

	surface_entry_t e = {
		.surface = input,
		.output = output_buffer,
		.list = buffers.da,
		.pts = enc->frame_cnt,
		.type = slice_type
	};

	if (!surface_queue_push_and_render(enc->surfq, &e))
		goto fail;

	enc->frame_cnt++;
	enc->gop_idx++;
	enc->gop_idx %= enc->intra_period;


	return true;

fail:
	destroy_buffers(enc, &buffers.da);
	da_free(buffers);
	return false;
}

bool upload_frame_to_surface(vaapi_encoder_t *enc, struct encoder_frame *frame,
		VASurfaceID surface)
{
	VAStatus status;
	VAImage image;
	void *data;

	CHECK_STATUS_FALSE(vaDeriveImage(enc->display, surface, &image));

	CHECK_STATUS_FAIL(vaMapBuffer(enc->display, image.buf, &data));
	for(uint32_t i = 0; i < 2; i++) {
		uint8_t *d_in = frame->data[i];
		uint8_t *d_out = data + image.offsets[i];
		for(uint32_t j = 0; j < enc->height / (i+1); j++) {
			memcpy(d_out, d_in, enc->width);
			d_in += frame->linesize[i];
			d_out += image.pitches[i];
		}
	}
	CHECK_STATUS_FAIL(vaUnmapBuffer(enc->display, image.buf));

	vaDestroyImage(enc->display, image.image_id);

	return true;

fail:
	vaDestroyImage(enc->display, image.image_id);

	return false;
}

bool vaapi_encoder_encode(vaapi_encoder_t *enc, struct encoder_frame *frame)
{
	VASurfaceID input_surface;

	vaapi_display_lock(enc->vdisplay);

	if (!surface_queue_pop_available(enc->surfq, &input_surface)) {
		VA_LOG(LOG_ERROR, "unable to aquire input surface");
		goto fail;
	}

	// todo: add profiling

	if (!upload_frame_to_surface(enc, frame, input_surface)) {
		VA_LOG(LOG_ERROR, "unable to upload frame to input surface");
		goto fail;
	}

	if (!encode_surface(enc, input_surface)) {
		VA_LOG(LOG_ERROR, "unable to encode frame");
		goto fail;
	}

	coded_block_entry_t c;
	bool success;
	if (!surface_queue_pop_finished(enc->surfq, &c, &success)) {
		VA_LOG(LOG_ERROR, "unable to pop finished frame");
		goto fail;
	}

	if (success)
		enc->coded_block_cb(enc->coded_block_cb_opaque, &c);

	vaapi_display_unlock(enc->vdisplay);

	return true;

fail:
	vaDestroySurfaces(enc->display, &input_surface, 1);

	vaapi_display_unlock(enc->vdisplay);

	return false;
}

#define CLAMP(val, minval, maxval) ((val > maxval) ? maxval : ((val < minval) ? minval : val))
#define SET_CLAMPED(val, minval, maxval) \
	if ((attribs->val > maxval) || (attribs->val < minval)) { \
		VA_LOG(LOG_WARNING, #val " out of range (%d >= %d <= %d), " \
				"clamping value", minval, \
				attribs->val, maxval); \
	} \
	enc->rc.val = CLAMP(attribs->val, minval, maxval);

void apply_rc_cbr(vaapi_encoder_t *enc, vaapi_encoder_attribs_t *attribs)
{
	vaapi_encoder_set_bitrate(enc, attribs->bitrate);

	if (attribs->use_custom_cpb)
		vaapi_encoder_set_cpb(enc, attribs->cpb_window_ms,
				attribs->cpb_size);
	else
		vaapi_encoder_set_cpb(enc, HRD_DEFAULT_CPB_WINDOW_SIZE,
				enc->rc.bitrate);


	enc->rc.qp = enc->rc.min_qp = 26;
	enc->rc.max_qp_delta = 0;
}

void apply_rc_cqp(vaapi_encoder_t *enc, vaapi_encoder_attribs_t *attribs)
{
	vaapi_encoder_set_bitrate(enc, 0);
	vaapi_encoder_set_cpb(enc, 0, 0);

	SET_CLAMPED(qp, 1, 51);
	enc->rc.min_qp = enc->rc.qp;
	enc->rc.max_qp_delta = 0;
}

uint32_t estimate_bitrate(vaapi_encoder_t *enc,
		vaapi_encoder_attribs_t *attribs)
{
	// Borrowed from gst-vaapi

	// According to the literature and testing, CABAC entropy coding
        // mode could provide for +10% to +18% improvement in general,
        // thus estimating +15% here ; and using adaptive 8x8 transforms
        // in I-frames could bring up to +10% improvement.
	uint32_t bits_per_mb = 48;

	// CABAC
	if (attribs->profile == VAAPI_PROFILE_BASELINE ||
	    attribs->profile == VAAPI_PROFILE_BASELINE_CONSTRAINED)
		bits_per_mb += (bits_per_mb * 15) / 100;

	// 8x8
	if (attribs->profile != VAAPI_PROFILE_HIGH)
		bits_per_mb += (bits_per_mb * 10) / 100;

	uint32_t bitrate = bits_per_mb *
			(enc->width_in_mbs * enc->height_in_mbs) *
			((float)enc->framerate_num / enc->framerate_den) /
			1000;

	return bitrate;
}

void apply_rc_vbr(vaapi_encoder_t *enc, vaapi_encoder_attribs_t *attribs)
{
	// fairly sure this has no effect
	vaapi_encoder_set_bitrate(enc, estimate_bitrate(enc, attribs));

	if (attribs->use_custom_cpb)
		vaapi_encoder_set_cpb(enc, attribs->cpb_window_ms,
				attribs->cpb_size);
	else
		vaapi_encoder_set_cpb(enc, HRD_DEFAULT_CPB_WINDOW_SIZE,
				enc->rc.bitrate);

	SET_CLAMPED(qp, 1, 51);
	SET_CLAMPED(min_qp, 1, 51);
	SET_CLAMPED(max_qp_delta, 1, 51);
}

bool apply_rate_control(vaapi_encoder_t *enc, vaapi_encoder_attribs_t *attribs)
{
	enc->rc.type = attribs->rc_type;
	enc->rc.use_custom_cpb = attribs->use_custom_cpb;

	switch(enc->rc.type) {
	case VAAPI_RC_CBR:
		apply_rc_cbr(enc, attribs);
		break;
	case VAAPI_RC_CQP:
		apply_rc_cqp(enc, attribs);
		break;
	case VAAPI_RC_VBR:
	case VAAPI_RC_VBR_CONSTRAINED:
		apply_rc_vbr(enc, attribs);
		break;
	default:
		return false;
	}

	return true;
}

static const char * vaapi_rc_to_str(vaapi_rc_t rc)
{
#define RC_CASE(x) case VAAPI_RC_ ## x: return #x
	switch (rc)
	{
	RC_CASE(CBR);
	RC_CASE(CQP);
	RC_CASE(VBR);
	RC_CASE(VBR_CONSTRAINED);
	default: return "Invalid RC";
	}
#undef RC_CASE
}

static void dump_encoder_info(vaapi_encoder_t *enc)
{
	DARRAY(char) info;

	static const char ZERO_BYTE = '\0';
	static const char NL_BYTE = '\n';

	da_init(info);

#define PUSHSTR(str) \
	da_push_back_array(info, str, strlen(str)); \
	da_push_back(info, &NL_BYTE);
#define PUSHFMT(format, arg) \
	do { \
		size_t size = snprintf(NULL, 0, format, arg) + 1; \
		char *str = bzalloc(size); \
		snprintf(str, size, format, arg); \
		da_push_back_array(info, str, size - 1); \
		da_push_back(info, &NL_BYTE); \
		bfree(str); \
	} while (false);

	PUSHSTR("settings:\n");
	PUSHFMT("\tdevice_type :          %s",
			(enc->vdisplay->type == VAAPI_DISPLAY_DRM) ?
					"DRM" : "X11");

	PUSHFMT("\tdevice:                %s", enc->vdisplay->name);

	PUSHFMT("\tdevice_path:           %s", enc->vdisplay->path);
	PUSHFMT("\tfps_num:               %d", enc->framerate_num);
	PUSHFMT("\tfps_den:               %d", enc->framerate_den);
	PUSHFMT("\twidth:                 %d", enc->width);
	PUSHFMT("\theight:                %d", enc->height);
	PUSHFMT("\tprofile:               %s", enc->caps->def.name);
	PUSHFMT("\tkeyint:                %d (s)", enc->keyint);
	PUSHFMT("\trc_type:               %s", vaapi_rc_to_str(enc->rc.type));

	if (enc->rc.type == VAAPI_RC_CBR)
		PUSHFMT("\tbitrate:               %d (kbit)",
				enc->rc.bitrate);

	if (enc->rc.type == VAAPI_RC_CBR ||
	    enc->rc.type == VAAPI_RC_VBR ||
	    enc->rc.type == VAAPI_RC_VBR_CONSTRAINED) {
		PUSHFMT("\tuse_custom_cpb:        %s", enc->rc.use_custom_cpb ?
				"yes" : "no");
		if (enc->rc.use_custom_cpb)
			PUSHFMT("\tcpb_size:              %d (kbit)",
					enc->rc.cpb_size);
	}

	if (enc->rc.type == VAAPI_RC_CQP ||
	    enc->rc.type == VAAPI_RC_VBR ||
	    enc->rc.type == VAAPI_RC_VBR_CONSTRAINED)
		PUSHFMT("\tqp:                    %d", enc->rc.qp);

	if (enc->rc.type == VAAPI_RC_VBR ||
	    enc->rc.type == VAAPI_RC_VBR_CONSTRAINED) {
		PUSHFMT("\tmin_qp:                %d", enc->rc.min_qp);
		PUSHFMT("\tmax_qp_delta:          %d", enc->rc.max_qp_delta);
	}


	da_push_back(info, &ZERO_BYTE);

	VA_LOG_STR(LOG_INFO, info.array);

	da_free(info);
}

vaapi_encoder_t *vaapi_encoder_create(vaapi_encoder_attribs_t *attribs)
{
	vaapi_encoder_t *enc;

	enc = bzalloc(sizeof(vaapi_encoder_t));

	enc->vdisplay = attribs->display;

	vaapi_display_lock(enc->vdisplay);

	// This display is currently closed
	if (!vaapi_display_open(enc->vdisplay)) {
		const char *name = enc->vdisplay->name;
		VA_LOG(LOG_ERROR, "unable to open '%s' device",
				!!name ? name : "Undefined");
		goto fail;
	}

	enc->display = attribs->display->display;

	if (enc->display == NULL)
		goto fail;

	enc->caps = vaapi_caps_from_profile(attribs->display, attribs->profile);
	if (enc->caps == NULL) {
		VA_LOG(LOG_ERROR, "failed to find any valid profiles for this "
				" hardware");
		goto fail;
	}

	enc->width                 = attribs->width;
	enc->height                = attribs->height;

	enc->width_in_mbs          = (enc->width + 15) / 16;
	enc->height_in_mbs         = (enc->height + 15) / 16;

	enc->framerate_num         = attribs->framerate_num;
	enc->framerate_den         = attribs->framerate_den;
	enc->keyint                = attribs->keyint;
	enc->format                = attribs->format;

	enc->surface_cnt           = attribs->surface_cnt;
	enc->coded_block_cb_opaque = attribs->coded_block_cb_opaque;
	enc->coded_block_cb        = attribs->coded_block_cb;

	enc->output_buf_size = enc->width * enc->height;

	da_resize(enc->refpics, attribs->refpic_cnt);

	float fps = (float)enc->framerate_num / enc->framerate_den;
	enc->intra_period = fps * enc->keyint;

	apply_rate_control(enc, attribs);

	if (!initialize_encoder(enc)) {
		VA_LOG(LOG_ERROR, "failed to initialize encoder for profile %s",
				enc->caps->def.name);
		goto fail;
	}

	dump_encoder_info(enc);

	enc->surfq = surface_queue_create(enc->display, enc->context,
			enc->surface_cnt, enc->width, enc->height);

	init_sps(enc);
	init_pps(enc);

	vaapi_display_unlock(enc->vdisplay);

	return enc;

fail:

	vaapi_display_unlock(enc->vdisplay);

	vaapi_encoder_destroy(enc);

	return NULL;
}
