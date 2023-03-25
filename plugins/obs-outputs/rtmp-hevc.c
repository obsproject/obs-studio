/******************************************************************************
    Copyright (C) 2023 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "rtmp-hevc.h"
#include "utils.h"

#include <obs.h>
#include <obs-nal.h>
#include <obs-hevc.h>
#include <util/array-serializer.h>

/* Adapted from FFmpeg's libavformat/hevc.c for our FLV muxer. */

enum {
	OBS_VPS_INDEX,
	OBS_SPS_INDEX,
	OBS_PPS_INDEX,
	OBS_SEI_PREFIX_INDEX,
	OBS_SEI_SUFFIX_INDEX,
	OBS_NB_ARRAYS
};

enum {
	OBS_HEVC_MAX_VPS_COUNT = 16,
	OBS_HEVC_MAX_SPS_COUNT = 16,
	OBS_HEVC_MAX_PPS_COUNT = 64,
};

typedef struct HVCCNALUnitArray {
	uint8_t array_completeness;
	uint8_t NAL_unit_type;
	uint16_t numNalus;
	struct array_output_data nalUnitData;
	struct serializer nalUnit;
} HVCCNALUnitArray;

typedef struct HEVCDecoderConfigurationRecord {
	uint8_t general_profile_space;
	uint8_t general_tier_flag;
	uint8_t general_profile_idc;
	uint32_t general_profile_compatibility_flags;
	uint64_t general_constraint_indicator_flags;
	uint8_t general_level_idc;
	uint16_t min_spatial_segmentation_idc;
	uint8_t parallelismType;
	uint8_t chromaFormat;
	uint8_t bitDepthLumaMinus8;
	uint8_t bitDepthChromaMinus8;
	uint16_t avgFrameRate;
	uint8_t constantFrameRate;
	uint8_t numTemporalLayers;
	uint8_t temporalIdNested;
	uint8_t lengthSizeMinusOne;
	uint8_t numOfArrays;
	HVCCNALUnitArray arrays[OBS_NB_ARRAYS];
} HEVCDecoderConfigurationRecord;

typedef struct HVCCProfileTierLevel {
	uint8_t profile_space;
	uint8_t tier_flag;
	uint8_t profile_idc;
	uint32_t profile_compatibility_flags;
	uint64_t constraint_indicator_flags;
	uint8_t level_idc;
} HVCCProfileTierLevel;

typedef struct HevcGetBitContext {
	const uint8_t *buffer, *buffer_end;
	uint64_t cache;
	unsigned bits_left;
	int index;
	int size_in_bits;
	int size_in_bits_plus8;
} HevcGetBitContext;

static inline uint32_t rb32(const uint8_t *ptr)
{
	return (ptr[0] << 24) + (ptr[1] << 16) + (ptr[2] << 8) + ptr[3];
}

static inline void refill_32(HevcGetBitContext *s)
{
	s->cache = s->cache | (uint64_t)rb32(s->buffer + (s->index >> 3))
				      << (32 - s->bits_left);
	s->index += 32;
	s->bits_left += 32;
}

static inline uint64_t get_val(HevcGetBitContext *s, unsigned n)
{
	uint64_t ret;
	ret = s->cache >> (64 - n);
	s->cache <<= n;
	s->bits_left -= n;
	return ret;
}

static inline int init_get_bits_xe(HevcGetBitContext *s, const uint8_t *buffer,
				   int bit_size)
{
	int buffer_size;
	int ret = 0;

	if (bit_size >= INT_MAX - 64 * 8 || bit_size < 0 || !buffer) {
		bit_size = 0;
		buffer = NULL;
		ret = -1;
	}

	buffer_size = (bit_size + 7) >> 3;

	s->buffer = buffer;
	s->size_in_bits = bit_size;
	s->size_in_bits_plus8 = bit_size + 8;
	s->buffer_end = buffer + buffer_size;
	s->index = 0;

	s->cache = 0;
	s->bits_left = 0;
	refill_32(s);

	return ret;
}

static inline int init_get_bits(HevcGetBitContext *s, const uint8_t *buffer,
				int bit_size)
{
	return init_get_bits_xe(s, buffer, bit_size);
}

static inline int init_get_bits8(HevcGetBitContext *s, const uint8_t *buffer,
				 int byte_size)
{
	if (byte_size > INT_MAX / 8 || byte_size < 0)
		byte_size = -1;
	return init_get_bits(s, buffer, byte_size * 8);
}

static inline unsigned int get_bits(HevcGetBitContext *s, unsigned int n)
{
	register unsigned int tmp;

	if (n > s->bits_left) {
		refill_32(s);
	}
	tmp = (unsigned int)get_val(s, n);
	return tmp;
}

#define skip_bits get_bits

static inline int get_bits_count(HevcGetBitContext *s)
{
	return s->index - s->bits_left;
}

static inline int get_bits_left(HevcGetBitContext *gb)
{
	return gb->size_in_bits - get_bits_count(gb);
}

static inline unsigned int get_bits_long(HevcGetBitContext *s, int n)
{
	if (!n)
		return 0;
	return get_bits(s, n);
}

#define skip_bits_long get_bits_long

static inline uint64_t get_bits64(HevcGetBitContext *s, int n)
{
	if (n <= 32) {
		return get_bits_long(s, n);
	} else {
		uint64_t ret = (uint64_t)get_bits_long(s, n - 32) << 32;
		return ret | get_bits_long(s, 32);
	}
}
static inline int ilog2(unsigned x)
{
	return (31 - clz32(x | 1));
}
static inline unsigned show_val(const HevcGetBitContext *s, unsigned n)
{
	return s->cache & ((UINT64_C(1) << n) - 1);
}
static inline unsigned int show_bits(HevcGetBitContext *s, unsigned int n)
{
	register unsigned int tmp;
	if (n > s->bits_left)
		refill_32(s);
	tmp = show_val(s, n);
	return tmp;
}
static inline unsigned int show_bits_long(HevcGetBitContext *s, int n)
{
	if (n <= 25) {
		return show_bits(s, n);
	} else {
		HevcGetBitContext gb = *s;
		return get_bits_long(&gb, n);
	}
}

static inline unsigned get_ue_golomb_long(HevcGetBitContext *gb)
{
	unsigned buf, log;

	buf = show_bits_long(gb, 32);
	log = 31 - ilog2(buf);
	skip_bits_long(gb, log);

	return get_bits_long(gb, log + 1) - 1;
}
static inline int get_se_golomb_long(HevcGetBitContext *gb)
{
	unsigned int buf = get_ue_golomb_long(gb);
	int sign = (buf & 1) - 1;
	return ((buf >> 1) ^ sign) + 1;
}

static inline bool has_start_code(const uint8_t *data, size_t size)
{
	if (size > 3 && data[0] == 0 && data[1] == 0 && data[2] == 1)
		return true;

	if (size > 4 && data[0] == 0 && data[1] == 0 && data[2] == 0 &&
	    data[3] == 1)
		return true;

	return false;
}

uint8_t *ff_nal_unit_extract_rbsp(uint8_t *dst, const uint8_t *src, int src_len,
				  uint32_t *dst_len, int header_len)
{
	int i, len;

	/* NAL unit header */
	i = len = 0;
	while (i < header_len && i < src_len)
		dst[len++] = src[i++];

	while (i + 2 < src_len)
		if (!src[i] && !src[i + 1] && src[i + 2] == 3) {
			dst[len++] = src[i++];
			dst[len++] = src[i++];
			i++; // remove emulation_prevention_three_byte
		} else
			dst[len++] = src[i++];

	while (i < src_len)
		dst[len++] = src[i++];

	memset(dst + len, 0, 64);

	*dst_len = (uint32_t)len;
	return dst;
}

static int hvcc_array_add_nal_unit(uint8_t *nal_buf, uint32_t nal_size,
				   uint8_t nal_type, int ps_array_completeness,
				   HVCCNALUnitArray *array)
{
	s_wb16(&array->nalUnit, nal_size);
	s_write(&array->nalUnit, nal_buf, nal_size);
	array->NAL_unit_type = nal_type;
	array->numNalus++;

	if (nal_type == OBS_HEVC_NAL_VPS || nal_type == OBS_HEVC_NAL_SPS ||
	    nal_type == OBS_HEVC_NAL_PPS)
		array->array_completeness = ps_array_completeness;

	return 0;
}

static void nal_unit_parse_header(HevcGetBitContext *gb, uint8_t *nal_type)
{
	skip_bits(gb, 1); // forbidden_zero_bit
	*nal_type = get_bits(gb, 6);
	get_bits(gb, 9);
}

static void hvcc_update_ptl(HEVCDecoderConfigurationRecord *hvcc,
			    HVCCProfileTierLevel *ptl)
{
	hvcc->general_profile_space = ptl->profile_space;
	if (hvcc->general_tier_flag < ptl->tier_flag)
		hvcc->general_level_idc = ptl->level_idc;
	else
		hvcc->general_level_idc =
			max_u8(hvcc->general_level_idc, ptl->level_idc);
	hvcc->general_tier_flag =
		max_u8(hvcc->general_tier_flag, ptl->tier_flag);
	hvcc->general_profile_idc =
		max_u8(hvcc->general_profile_idc, ptl->profile_idc);
	hvcc->general_profile_compatibility_flags &=
		ptl->profile_compatibility_flags;
	hvcc->general_constraint_indicator_flags &=
		ptl->constraint_indicator_flags;
}

static void hvcc_parse_ptl(HevcGetBitContext *gb,
			   HEVCDecoderConfigurationRecord *hvcc,
			   unsigned int max_sub_layers_minus1)
{
	unsigned int i;
	HVCCProfileTierLevel general_ptl;
	uint8_t sub_layer_profile_present_flag[7]; // max sublayers
	uint8_t sub_layer_level_present_flag[7];   // max sublayers

	general_ptl.profile_space = get_bits(gb, 2);
	general_ptl.tier_flag = get_bits(gb, 1);
	general_ptl.profile_idc = get_bits(gb, 5);
	general_ptl.profile_compatibility_flags = get_bits_long(gb, 32);
	general_ptl.constraint_indicator_flags = get_bits64(gb, 48);
	general_ptl.level_idc = get_bits(gb, 8);
	hvcc_update_ptl(hvcc, &general_ptl);

	for (i = 0; i < max_sub_layers_minus1; i++) {
		sub_layer_profile_present_flag[i] = get_bits(gb, 1);
		sub_layer_level_present_flag[i] = get_bits(gb, 1);
	}

	// skip the rest
	if (max_sub_layers_minus1 > 0)
		for (i = max_sub_layers_minus1; i < 8; i++)
			skip_bits(gb, 2);

	for (i = 0; i < max_sub_layers_minus1; i++) {
		if (sub_layer_profile_present_flag[i]) {
			skip_bits_long(gb, 32);
			skip_bits_long(gb, 32);
			skip_bits(gb, 24);
		}

		if (sub_layer_level_present_flag[i])
			skip_bits(gb, 8);
	}
}

static int hvcc_parse_vps(HevcGetBitContext *gb,
			  HEVCDecoderConfigurationRecord *hvcc)
{
	unsigned int vps_max_sub_layers_minus1;
	skip_bits(gb, 12);
	vps_max_sub_layers_minus1 = get_bits(gb, 3);
	hvcc->numTemporalLayers =
		max_u8(hvcc->numTemporalLayers, vps_max_sub_layers_minus1 + 1);
	skip_bits(gb, 17);
	hvcc_parse_ptl(gb, hvcc, vps_max_sub_layers_minus1);
	return 0;
}

#define HEVC_MAX_SHORT_TERM_REF_PIC_SETS 64

static void skip_scaling_list_data(HevcGetBitContext *gb)
{
	int i, j, k, num_coeffs;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < (i == 3 ? 2 : 6); j++) {
			if (!get_bits(gb, 1))
				get_ue_golomb_long(gb);
			else {
				num_coeffs = min_i32(64, 1 << (4 + (i << 1)));

				if (i > 1)
					get_se_golomb_long(gb);
				for (k = 0; k < num_coeffs; k++)
					get_se_golomb_long(gb);
			}
		}
	}
}

static int
parse_rps(HevcGetBitContext *gb, unsigned int rps_idx, unsigned int num_rps,
	  unsigned int num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS])
{
	unsigned int i;

	if (rps_idx && get_bits(gb, 1)) { // inter_ref_pic_set_prediction_flag
		/* this should only happen for slice headers, and this isn't one */
		if (rps_idx >= num_rps)
			return -1;

		get_bits(gb, 1);        // delta_rps_sign
		get_ue_golomb_long(gb); // abs_delta_rps_minus1

		num_delta_pocs[rps_idx] = 0;

		for (i = 0; i <= num_delta_pocs[rps_idx - 1]; i++) {
			uint8_t use_delta_flag = 0;
			uint8_t used_by_curr_pic_flag = get_bits(gb, 1);
			if (!used_by_curr_pic_flag)
				use_delta_flag = get_bits(gb, 1);

			if (used_by_curr_pic_flag || use_delta_flag)
				num_delta_pocs[rps_idx]++;
		}
	} else {
		unsigned int num_negative_pics = get_ue_golomb_long(gb);
		unsigned int num_positive_pics = get_ue_golomb_long(gb);

		if ((num_positive_pics + (uint64_t)num_negative_pics) * 2 >
		    (uint64_t)get_bits_left(gb))
			return -1;

		num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

		for (i = 0; i < num_negative_pics; i++) {
			get_ue_golomb_long(gb); // delta_poc_s0_minus1[rps_idx]
			get_bits(gb, 1); // used_by_curr_pic_s0_flag[rps_idx]
		}

		for (i = 0; i < num_positive_pics; i++) {
			get_ue_golomb_long(gb); // delta_poc_s1_minus1[rps_idx]
			get_bits(gb, 1); // used_by_curr_pic_s1_flag[rps_idx]
		}
	}

	return 0;
}

static void
skip_sub_layer_hrd_parameters(HevcGetBitContext *gb,
			      unsigned int cpb_cnt_minus1,
			      uint8_t sub_pic_hrd_params_present_flag)
{
	unsigned int i;

	for (i = 0; i <= cpb_cnt_minus1; i++) {
		get_ue_golomb_long(gb); // bit_rate_value_minus1
		get_ue_golomb_long(gb); // cpb_size_value_minus1

		if (sub_pic_hrd_params_present_flag) {
			get_ue_golomb_long(gb); // cpb_size_du_value_minus1
			get_ue_golomb_long(gb); // bit_rate_du_value_minus1
		}

		get_bits(gb, 1); // cbr_flag
	}
}

static int skip_hrd_parameters(HevcGetBitContext *gb,
			       uint8_t cprms_present_flag,
			       unsigned int max_sub_layers_minus1)
{
	unsigned int i;
	uint8_t sub_pic_hrd_params_present_flag = 0;
	uint8_t nal_hrd_parameters_present_flag = 0;
	uint8_t vcl_hrd_parameters_present_flag = 0;

	if (cprms_present_flag) {
		nal_hrd_parameters_present_flag = get_bits(gb, 1);
		vcl_hrd_parameters_present_flag = get_bits(gb, 1);

		if (nal_hrd_parameters_present_flag ||
		    vcl_hrd_parameters_present_flag) {
			sub_pic_hrd_params_present_flag = get_bits(gb, 1);

			if (sub_pic_hrd_params_present_flag)
				get_bits(gb, 19);

			get_bits(gb, 8);

			if (sub_pic_hrd_params_present_flag)
				get_bits(gb, 4);
			get_bits(gb, 15);
		}
	}

	for (i = 0; i <= max_sub_layers_minus1; i++) {
		unsigned int cpb_cnt_minus1 = 0;
		uint8_t low_delay_hrd_flag = 0;
		uint8_t fixed_pic_rate_within_cvs_flag = 0;
		uint8_t fixed_pic_rate_general_flag = get_bits(gb, 1);

		if (!fixed_pic_rate_general_flag)
			fixed_pic_rate_within_cvs_flag = get_bits(gb, 1);

		if (fixed_pic_rate_within_cvs_flag)
			get_ue_golomb_long(gb);
		else
			low_delay_hrd_flag = get_bits(gb, 1);

		if (!low_delay_hrd_flag) {
			cpb_cnt_minus1 = get_ue_golomb_long(gb);
			if (cpb_cnt_minus1 > 31)
				return -1;
		}

		if (nal_hrd_parameters_present_flag)
			skip_sub_layer_hrd_parameters(
				gb, cpb_cnt_minus1,
				sub_pic_hrd_params_present_flag);

		if (vcl_hrd_parameters_present_flag)
			skip_sub_layer_hrd_parameters(
				gb, cpb_cnt_minus1,
				sub_pic_hrd_params_present_flag);
	}

	return 0;
}

static void hvcc_parse_vui(HevcGetBitContext *gb,
			   HEVCDecoderConfigurationRecord *hvcc,
			   unsigned int max_sub_layers_minus1)
{
	unsigned int min_spatial_segmentation_idc;

	if (get_bits(gb, 1))                // aspect_ratio_info_present_flag
		if (get_bits(gb, 8) == 255) // aspect_ratio_idc
			get_bits_long(gb,
				      32); // sar_width u(16), sar_height u(16)

	if (get_bits(gb, 1))     // overscan_info_present_flag
		get_bits(gb, 1); // overscan_appropriate_flag

	if (get_bits(gb, 1)) { // video_signal_type_present_flag
		get_bits(gb,
			 4); // video_format u(3), video_full_range_flag u(1)

		if (get_bits(gb, 1)) // colour_description_present_flag
			get_bits(gb, 24);
	}

	if (get_bits(gb, 1)) {
		get_ue_golomb_long(gb);
		get_ue_golomb_long(gb);
	}

	get_bits(gb, 3);

	if (get_bits(gb, 1)) {          // default_display_window_flag
		get_ue_golomb_long(gb); // def_disp_win_left_offset
		get_ue_golomb_long(gb); // def_disp_win_right_offset
		get_ue_golomb_long(gb); // def_disp_win_top_offset
		get_ue_golomb_long(gb); // def_disp_win_bottom_offset
	}

	if (get_bits(gb, 1)) { // vui_timing_info_present_flag
		// skip timing info
		get_bits_long(gb, 32); // num_units_in_tick
		get_bits_long(gb, 32); // time_scale

		if (get_bits(gb, 1)) // poc_proportional_to_timing_flag
			get_ue_golomb_long(gb); // num_ticks_poc_diff_one_minus1

		if (get_bits(gb, 1)) // vui_hrd_parameters_present_flag
			skip_hrd_parameters(gb, 1, max_sub_layers_minus1);
	}

	if (get_bits(gb, 1)) { // bitstream_restriction_flag
		get_bits(gb, 3);

		min_spatial_segmentation_idc = get_ue_golomb_long(gb);
		hvcc->min_spatial_segmentation_idc =
			min_u16(hvcc->min_spatial_segmentation_idc,
				min_spatial_segmentation_idc);

		get_ue_golomb_long(gb); // max_bytes_per_pic_denom
		get_ue_golomb_long(gb); // max_bits_per_min_cu_denom
		get_ue_golomb_long(gb); // log2_max_mv_length_horizontal
		get_ue_golomb_long(gb); // log2_max_mv_length_vertical
	}
}

static int hvcc_parse_sps(HevcGetBitContext *gb,
			  HEVCDecoderConfigurationRecord *hvcc)
{
	unsigned int i, sps_max_sub_layers_minus1,
		log2_max_pic_order_cnt_lsb_minus4;
	unsigned int num_short_term_ref_pic_sets,
		num_delta_pocs[HEVC_MAX_SHORT_TERM_REF_PIC_SETS];

	get_bits(gb, 4); // sps_video_parameter_set_id
	sps_max_sub_layers_minus1 = get_bits(gb, 3);

	hvcc->numTemporalLayers =
		max_u8(hvcc->numTemporalLayers, sps_max_sub_layers_minus1 + 1);

	hvcc->temporalIdNested = get_bits(gb, 1);

	hvcc_parse_ptl(gb, hvcc, sps_max_sub_layers_minus1);

	get_ue_golomb_long(gb); // sps_seq_parameter_set_id

	hvcc->chromaFormat = get_ue_golomb_long(gb);

	if (hvcc->chromaFormat == 3)
		get_bits(gb, 1); // separate_colour_plane_flag

	get_ue_golomb_long(gb); // pic_width_in_luma_samples
	get_ue_golomb_long(gb); // pic_height_in_luma_samples

	if (get_bits(gb, 1)) {          // conformance_window_flag
		get_ue_golomb_long(gb); // conf_win_left_offset
		get_ue_golomb_long(gb); // conf_win_right_offset
		get_ue_golomb_long(gb); // conf_win_top_offset
		get_ue_golomb_long(gb); // conf_win_bottom_offset
	}

	hvcc->bitDepthLumaMinus8 = get_ue_golomb_long(gb);
	hvcc->bitDepthChromaMinus8 = get_ue_golomb_long(gb);
	log2_max_pic_order_cnt_lsb_minus4 = get_ue_golomb_long(gb);

	/* sps_sub_layer_ordering_info_present_flag */
	i = get_bits(gb, 1) ? 0 : sps_max_sub_layers_minus1;
	for (; i <= sps_max_sub_layers_minus1; i++) {
		get_ue_golomb_long(gb); // max_dec_pic_buffering_minus1
		get_ue_golomb_long(gb); // max_num_reorder_pics
		get_ue_golomb_long(gb); // max_latency_increase_plus1
	}

	get_ue_golomb_long(gb); // log2_min_luma_coding_block_size_minus3
	get_ue_golomb_long(gb); // log2_diff_max_min_luma_coding_block_size
	get_ue_golomb_long(gb); // log2_min_transform_block_size_minus2
	get_ue_golomb_long(gb); // log2_diff_max_min_transform_block_size
	get_ue_golomb_long(gb); // max_transform_hierarchy_depth_inter
	get_ue_golomb_long(gb); // max_transform_hierarchy_depth_intra

	if (get_bits(gb, 1) && // scaling_list_enabled_flag
	    get_bits(gb, 1))   // sps_scaling_list_data_present_flag
		skip_scaling_list_data(gb);

	get_bits(gb, 1); // amp_enabled_flag
	get_bits(gb, 1); // sample_adaptive_offset_enabled_flag

	if (get_bits(gb, 1)) {   // pcm_enabled_flag
		get_bits(gb, 4); // pcm_sample_bit_depth_luma_minus1
		get_bits(gb, 4); // pcm_sample_bit_depth_chroma_minus1
		get_ue_golomb_long(
			gb); // log2_min_pcm_luma_coding_block_size_minus3
		get_ue_golomb_long(
			gb);     // log2_diff_max_min_pcm_luma_coding_block_size
		get_bits(gb, 1); // pcm_loop_filter_disabled_flag
	}

	num_short_term_ref_pic_sets = get_ue_golomb_long(gb);
	if (num_short_term_ref_pic_sets > HEVC_MAX_SHORT_TERM_REF_PIC_SETS)
		return -1;

	for (i = 0; i < num_short_term_ref_pic_sets; i++) {
		int ret = parse_rps(gb, i, num_short_term_ref_pic_sets,
				    num_delta_pocs);
		if (ret < 0)
			return ret;
	}

	if (get_bits(gb, 1)) { // long_term_ref_pics_present_flag
		unsigned num_long_term_ref_pics_sps = get_ue_golomb_long(gb);
		if (num_long_term_ref_pics_sps > 31U)
			return -1;
		for (i = 0; i < num_long_term_ref_pics_sps;
		     i++) { // num_long_term_ref_pics_sps
			int len = min_i32(log2_max_pic_order_cnt_lsb_minus4 + 4,
					  16);
			get_bits(gb, len); // lt_ref_pic_poc_lsb_sps[i]
			get_bits(gb, 1);   // used_by_curr_pic_lt_sps_flag[i]
		}
	}

	get_bits(gb, 1); // sps_temporal_mvp_enabled_flag
	get_bits(gb, 1); // strong_intra_smoothing_enabled_flag

	if (get_bits(gb, 1)) // vui_parameters_present_flag
		hvcc_parse_vui(gb, hvcc, sps_max_sub_layers_minus1);

	/* nothing useful for hvcC past this point */
	return 0;
}

static int hvcc_parse_pps(HevcGetBitContext *gb,
			  HEVCDecoderConfigurationRecord *hvcc)
{
	uint8_t tiles_enabled_flag, entropy_coding_sync_enabled_flag;

	get_ue_golomb_long(gb); // pps_pic_parameter_set_id
	get_ue_golomb_long(gb); // pps_seq_parameter_set_id

	get_bits(gb, 7);

	get_ue_golomb_long(gb); // num_ref_idx_l0_default_active_minus1
	get_ue_golomb_long(gb); // num_ref_idx_l1_default_active_minus1
	get_se_golomb_long(gb); // init_qp_minus26

	get_bits(gb, 2);

	if (get_bits(gb, 1))            // cu_qp_delta_enabled_flag
		get_ue_golomb_long(gb); // diff_cu_qp_delta_depth

	get_se_golomb_long(gb); // pps_cb_qp_offset
	get_se_golomb_long(gb); // pps_cr_qp_offset

	get_bits(gb, 4);

	tiles_enabled_flag = get_bits(gb, 1);
	entropy_coding_sync_enabled_flag = get_bits(gb, 1);

	if (entropy_coding_sync_enabled_flag && tiles_enabled_flag)
		hvcc->parallelismType = 0; // mixed-type parallel decoding
	else if (entropy_coding_sync_enabled_flag)
		hvcc->parallelismType = 3; // wavefront-based parallel decoding
	else if (tiles_enabled_flag)
		hvcc->parallelismType = 2; // tile-based parallel decoding
	else
		hvcc->parallelismType = 1; // slice-based parallel decoding

	/* nothing useful for hvcC past this point */
	return 0;
}

static int hvcc_add_nal_unit(uint8_t *nal_buf, uint32_t nal_size,
			     int ps_array_completeness,
			     HEVCDecoderConfigurationRecord *hvcc,
			     unsigned array_idx)
{
	int ret = 0;
	HevcGetBitContext gbc;
	uint8_t nal_type;
	uint8_t *rbsp_buf;
	uint32_t rbsp_size;

	uint8_t *dst;
	dst = bmalloc(nal_size + 64);

	rbsp_buf =
		ff_nal_unit_extract_rbsp(dst, nal_buf, nal_size, &rbsp_size, 2);
	if (!rbsp_buf) {
		ret = -1;
		goto end;
	}

	ret = init_get_bits8(&gbc, rbsp_buf, rbsp_size);
	if (ret < 0)
		goto end;

	nal_unit_parse_header(&gbc, &nal_type);

	ret = hvcc_array_add_nal_unit(nal_buf, nal_size, nal_type,
				      ps_array_completeness,
				      &hvcc->arrays[array_idx]);
	if (ret < 0)
		goto end;
	if (hvcc->arrays[array_idx].numNalus == 1)
		hvcc->numOfArrays++;
	if (nal_type == OBS_HEVC_NAL_VPS)
		ret = hvcc_parse_vps(&gbc, hvcc);
	else if (nal_type == OBS_HEVC_NAL_SPS)
		ret = hvcc_parse_sps(&gbc, hvcc);
	else if (nal_type == OBS_HEVC_NAL_PPS)
		ret = hvcc_parse_pps(&gbc, hvcc);
	if (ret < 0)
		goto end;

end:
	bfree(dst);
	return ret;
}

size_t obs_parse_hevc_header(uint8_t **header, const uint8_t *data, size_t size)
{
	const uint8_t *start;
	const uint8_t *end;

	if (!has_start_code(data, size)) {
		*header = bmemdup(data, size);
		return size;
	}

	if (size < 6)
		return 0; // invalid
	if (*data == 1) { // already hvcC-formatted
		*header = bmemdup(data, size);
		return size;
	}

	struct array_output_data nals;
	struct serializer sn;
	array_output_serializer_init(&sn, &nals);

	const uint8_t *nal_start, *nal_end;
	start = data;
	end = data + size;

	size = 0; // reset size
	nal_start = obs_nal_find_startcode(start, end);
	for (;;) {
		while (nal_start < end && !*(nal_start++))
			;
		if (nal_start == end)
			break;
		nal_end = obs_nal_find_startcode(nal_start, end);

		assert(nal_end - nal_start <= INT_MAX);
		s_wb32(&sn, (uint32_t)(nal_end - nal_start));
		s_write(&sn, nal_start, nal_end - nal_start);
		size += 4 + nal_end - nal_start;
		nal_start = nal_end;
	}
	if (size == 0)
		goto done;

	start = nals.bytes.array;
	end = nals.bytes.array + nals.bytes.num;

	// HVCC init
	HEVCDecoderConfigurationRecord hvcc;
	memset(&hvcc, 0, sizeof(HEVCDecoderConfigurationRecord));
	hvcc.lengthSizeMinusOne = 3;                           // 4 bytes
	hvcc.general_profile_compatibility_flags = 0xffffffff; // all bits set
	hvcc.general_constraint_indicator_flags =
		0xffffffffffff;                       // all bits set
	hvcc.min_spatial_segmentation_idc = 4096 + 1; // assume invalid value
	for (unsigned i = 0; i < OBS_NB_ARRAYS; i++) {
		HVCCNALUnitArray *const array = &hvcc.arrays[i];
		array_output_serializer_init(&array->nalUnit,
					     &array->nalUnitData);
	}

	uint8_t *buf = (uint8_t *)start;
	while (end - buf > 4) {
		uint32_t len = rb32(buf);
		assert((end - buf - 4) <= INT_MAX);
		len = min_u32(len, (uint32_t)(end - buf - 4));
		uint8_t type = (buf[4] >> 1) & 0x3f;

		buf += 4;

		for (unsigned i = 0; i < OBS_NB_ARRAYS; i++) {
			static const uint8_t array_idx_to_type[] = {
				OBS_HEVC_NAL_VPS, OBS_HEVC_NAL_SPS,
				OBS_HEVC_NAL_PPS, OBS_HEVC_NAL_SEI_PREFIX,
				OBS_HEVC_NAL_SEI_SUFFIX};

			if (type == array_idx_to_type[i]) {
				int ps_array_completeness = 0;
				int ret = hvcc_add_nal_unit(
					buf, len, ps_array_completeness, &hvcc,
					i);
				if (ret < 0)
					goto free;
				break;
			}
		}

		buf += len;
	}

	// write hvcc data
	uint16_t vps_count, sps_count, pps_count;
	if (hvcc.min_spatial_segmentation_idc > 4096) // invalid?
		hvcc.min_spatial_segmentation_idc = 0;
	if (!hvcc.min_spatial_segmentation_idc)
		hvcc.parallelismType = 0;
	hvcc.avgFrameRate = 0;
	hvcc.constantFrameRate = 0;

	vps_count = hvcc.arrays[OBS_VPS_INDEX].numNalus;
	sps_count = hvcc.arrays[OBS_SPS_INDEX].numNalus;
	pps_count = hvcc.arrays[OBS_PPS_INDEX].numNalus;
	if (!vps_count || vps_count > OBS_HEVC_MAX_VPS_COUNT || !sps_count ||
	    sps_count > OBS_HEVC_MAX_SPS_COUNT || !pps_count ||
	    pps_count > OBS_HEVC_MAX_PPS_COUNT)
		goto free;

	struct array_output_data output;
	struct serializer s;
	array_output_serializer_init(&s, &output);
	s_w8(&s, 1); // configurationVersion, always 1

	s_w8(&s, hvcc.general_profile_space << 6 | hvcc.general_tier_flag << 5 |
			 hvcc.general_profile_idc);

	s_wb32(&s, hvcc.general_profile_compatibility_flags);

	s_wb32(&s, (uint32_t)(hvcc.general_constraint_indicator_flags >> 16));
	s_wb16(&s, (uint16_t)(hvcc.general_constraint_indicator_flags));

	s_w8(&s, hvcc.general_level_idc);

	s_wb16(&s, hvcc.min_spatial_segmentation_idc | 0xf000);
	s_w8(&s, hvcc.parallelismType | 0xfc);
	s_w8(&s, hvcc.chromaFormat | 0xfc);
	s_w8(&s, hvcc.bitDepthLumaMinus8 | 0xf8);
	s_w8(&s, hvcc.bitDepthChromaMinus8 | 0xf8);
	s_wb16(&s, hvcc.avgFrameRate);

	s_w8(&s, hvcc.constantFrameRate << 6 | hvcc.numTemporalLayers << 3 |
			 hvcc.temporalIdNested << 2 | hvcc.lengthSizeMinusOne);

	s_w8(&s, hvcc.numOfArrays);
	for (unsigned i = 0; i < OBS_NB_ARRAYS; i++) {
		const HVCCNALUnitArray *const array = &hvcc.arrays[i];

		if (!array->numNalus)
			continue;

		s_w8(&s, (array->array_completeness << 7) |
				 (array->NAL_unit_type & 0x3f));
		s_wb16(&s, array->numNalus);

		s_write(&s, array->nalUnitData.bytes.array,
			array->nalUnitData.bytes.num);
	}

	*header = output.bytes.array;
	size = output.bytes.num;

free:
	for (unsigned i = 0; i < OBS_NB_ARRAYS; i++) {
		HVCCNALUnitArray *const array = &hvcc.arrays[i];
		array->numNalus = 0;
		array_output_serializer_free(&array->nalUnitData);
	}

done:
	array_output_serializer_free(&nals);
	return size;
}
