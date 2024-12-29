/******************************************************************************
    Copyright (C) 2024 by Dennis Sädtler <dennis@obsproject.com>

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

#include "mp4-mux-internal.h"

#include "rtmp-hevc.h"
#include "rtmp-av1.h"

#include <obs-avc.h>
#include <obs-hevc.h>
#include <obs-module.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/array-serializer.h>

#include <time.h>

/*
 * (Mostly) compliant MP4 muxer for fun and profit.
 * Based on ISO/IEC 14496-12 and FFmpeg's libavformat/movenc.c ([L]GPL)
 *
 * Specification section numbers are noted where applicable.
 * Standard identifier is included if not referring to ISO/IEC 14496-12.
 */

#define do_log(level, format, ...)                                                          \
	blog(level, "[%s muxer: '%s'] " format, mux->flavor == FLAVOR_MOV ? "mov" : "mp4", \
	     obs_output_get_name(mux->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

/* Helper to overwrite placeholder size and return total size. */
static inline size_t write_box_size(struct serializer *s, int64_t start)
{
	int64_t end = serializer_get_pos(s);
	size_t size = end - start;

	serializer_seek(s, start, SERIALIZE_SEEK_START);
	s_wb32(s, (uint32_t)size);
	serializer_seek(s, end, SERIALIZE_SEEK_START);

	return size;
}

/// 4.2 Box header with size and char[4] name
static inline void write_box(struct serializer *s, const size_t size, const char name[4])
{
	if (size <= UINT32_MAX) {
		s_wb32(s, (uint32_t)size); // size
		s_write(s, name, 4);       // boxtype
	} else {
		s_wb32(s, 1);        // size
		s_write(s, name, 4); // boxtype
		s_wb64(s, size);     // largesize
	}
}

/// 4.2 FullBox extended header with u8 version and u24 flags
static inline void write_fullbox(struct serializer *s, const size_t size, const char name[4], uint8_t version,
				 uint32_t flags)
{
	write_box(s, size, name);
	s_w8(s, version);
	s_wb24(s, flags);
}

/// 4.3 File Type Box
static size_t mp4_write_ftyp(struct mp4_mux *mux, bool fragmented)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "ftyp");

	if (mux->flavor == FLAVOR_MOV) {
		/* For MOV, the brand is just "qt" followed by two spaces. */
		s_write(s, "qt  ", 4); // major brand
		s_wb32(s, 0x20140200); // minor version (BCD YYYYMM00 per QTFF spec)
		s_write(s, "qt  ", 4); // minor brand
	} else {
		const char *major_brand = "isom";
		/* Following FFmpeg's example, when using negative CTS the major brand
		 * needs to be either iso4 or iso6 depending on whether the file is
		 * currently fragmented. */
		if (mux->flags & MP4_USE_NEGATIVE_CTS)
			major_brand = fragmented ? "iso6" : "iso4";

		s_write(s, major_brand, 4); // major brand
		s_wb32(s, 0);               // minor version
		s_write(s, major_brand, 4); // minor brands (first one matches major brand)

		/* Write isom base brand if it's not the major brand */
		if (strcmp(major_brand, "isom") != 0)
			s_write(s, "isom", 4);

		/* Avoid adding newer brand (iso6) unless necessary, use "obs1" brand
		 * as a placeholder to maintain ftyp box size. */
		if (fragmented && strcmp(major_brand, "iso6") != 0)
			s_write(s, "iso6", 4);
		else
			s_write(s, "obs1", 4);

		s_write(s, "iso2", 4);

		/* Include H.264 brand if used */
		for (size_t i = 0; i < mux->tracks.num; i++) {
			struct mp4_track *track = &mux->tracks.array[i];
			if (track->type == TRACK_VIDEO) {
				if (track->codec == CODEC_H264)
					s_write(s, "avc1", 4);
				break;
			}
		}

		/* General MP4 brannd */
		s_write(s, "mp41", 4);
	}

	return write_box_size(s, start);
}

/// 8.1.2 Free Space Box
static size_t mp4_write_free(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	/* Write a 16-byte free box, so it can be replaced with a 64-bit size
	 * box header (u32 + char[4] + u64) */
	s_wb32(s, 16);
	s_write(s, mux->flavor == FLAVOR_MOV ? "wide" : "free", 4);
	s_wb64(s, 0);

	return 16;
}

/// 8.2.2 Movie Header Box
static size_t mp4_write_mvhd(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	size_t start = serializer_get_pos(s);

	/* Use primary video track as the baseline for duration */
	uint64_t duration = 0;
	for (size_t i = 0; i < mux->tracks.num; i++) {
		struct mp4_track *track = &mux->tracks.array[i];
		if (track->type == TRACK_VIDEO) {
			duration = util_mul_div64(track->duration, 1000, track->timebase_den);
			break;
		}
	}
	bool extended_ts = duration > UINT32_MAX || mux->creation_time > UINT32_MAX;
	uint8_t version = extended_ts ? 1 : 0;

	write_fullbox(s, 0, "mvhd", version, 0);

	if (extended_ts) {
		s_wb64(s, mux->creation_time); // creation time
		s_wb64(s, mux->creation_time); // modification time
		s_wb32(s, 1000);               // timescale
		s_wb64(s, duration);           // duration (0 for fragmented)
	} else {
		s_wb32(s, (uint32_t)mux->creation_time); // creation time
		s_wb32(s, (uint32_t)mux->creation_time); // modification time
		s_wb32(s, 1000);                         // timescale
		s_wb32(s, (uint32_t)duration);           // duration (0 for fragmented)
	}

	s_wb32(s, 0x00010000); // rate, 16.16 fixed float (1 << 16)
	s_wb16(s, 0x0100);     // volume

	s_wb16(s, 0); // reserved
	s_wb32(s, 0); // reserved
	s_wb32(s, 0); // reserved

	// Matrix
	for (int i = 0; i < 9; i++)
		s_wb32(s, UNITY_MATRIX[i]);

	// pre_defined
	s_wb32(s, 0);
	s_wb32(s, 0);
	s_wb32(s, 0);
	s_wb32(s, 0);
	s_wb32(s, 0);
	s_wb32(s, 0);

	s_wb32(s, mux->track_ctr + 1); // next_track_ID

	return write_box_size(s, start);
}

/// 8.3.2 Track Header Box
static size_t mp4_write_tkhd(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	size_t start = serializer_get_pos(s);

	uint64_t duration = util_mul_div64(track->duration, 1000, track->timebase_den);
	bool extended_ts = duration > UINT32_MAX || mux->creation_time > UINT32_MAX;
	uint8_t version = extended_ts ? 1 : 0;

	/* Flags are 0x1 (enabled) | 0x2 (in movie) */
	static const uint32_t flags = 0x1 | 0x2;
	write_fullbox(s, 0, "tkhd", version, flags);

	if (extended_ts) {
		s_wb64(s, mux->creation_time); // creation time
		s_wb64(s, mux->creation_time); // modification time
		s_wb32(s, track->track_id);    // track_id
		s_wb32(s, 0);                  // reserved
		s_wb64(s, duration);           // duration in movie timescale
	} else {
		s_wb32(s, (uint32_t)mux->creation_time); // creation time
		s_wb32(s, (uint32_t)mux->creation_time); // modification time
		s_wb32(s, track->track_id);              // track_id
		s_wb32(s, 0);                            // reserved
		s_wb32(s, (uint32_t)duration);           // duration in movie timescale
	}

	s_wb32(s, 0);                                      // reserved
	s_wb32(s, 0);                                      // reserved
	s_wb16(s, 0);                                      // layer
	s_wb16(s, track->type == TRACK_AUDIO ? 1 : 0);     // alternate group
	s_wb16(s, track->type == TRACK_AUDIO ? 0x100 : 0); // volume
	s_wb16(s, 0);                                      // reserved

	// Matrix (predefined)
	for (int i = 0; i < 9; i++)
		s_wb32(s, UNITY_MATRIX[i]);

	if (track->type == TRACK_AUDIO) {
		s_wb32(s, 0); // width
		s_wb32(s, 0); // height
	} else {
		/* width/height are fixed point 16.16, so we just shift the
		 * integer to the upper 16 bits */
		uint32_t width = obs_encoder_get_width(track->encoder);
		s_wb32(s, width << 16);
		uint32_t height = obs_encoder_get_height(track->encoder);
		s_wb32(s, height << 16);
	}

	return write_box_size(s, start);
}

/// 8.4.2 Media Header Box
static size_t mp4_write_mdhd(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;

	size_t size = 32;
	uint8_t version = 0;
	uint64_t duration = track->duration;
	uint32_t timescale = track->timescale;

	if (track->type == TRACK_VIDEO) {
		/* Update to track timescale */
		duration = util_mul_div64(duration, track->timescale, track->timebase_den);
	}

	/* use 64-bit duration if necessary */
	if (duration > UINT32_MAX || mux->creation_time > UINT32_MAX) {
		if (mux->flavor == FLAVOR_MOV) {
			/* QTFF does not specify how to handle 32-bit overflow for duration/timestamps. */
			warn("Duration too large for MOV, this file may be unplayable in QuickTime!");
		}

		size = 44;
		version = 1;
	}

	write_fullbox(s, size, "mdhd", version, 0);

	if (version == 1) {
		s_wb64(s, mux->creation_time); // creation time
		s_wb64(s, mux->creation_time); // modification time
		s_wb32(s, timescale);          // timescale
		s_wb64(s, (uint32_t)duration); // duration
	} else {
		s_wb32(s, (uint32_t)mux->creation_time); // creation time
		s_wb32(s, (uint32_t)mux->creation_time); // modification time
		s_wb32(s, timescale);                    // timescale
		s_wb32(s, (uint32_t)duration);           // duration
	}

	s_wb16(s, mux->flavor == FLAVOR_MOV ? 32767 : 21956); // language (undefined)
	s_wb16(s, 0);                                         // pre_defined

	return size;
}

/// 8.4.3 Handler Reference Box
static size_t mp4_write_hdlr(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "hdlr", 0, 0);

	if (mux->flavor == FLAVOR_MOV)
		s_write(s, track ? "mhlr" : "dhlr", 4);
	else
		s_wb32(s, 0); // pre_defined

	// handler_type
	if (!track)
		s_write(s, "url ", 4);
	else if (track->type == TRACK_VIDEO)
		s_write(s, "vide", 4);
	else if (track->type == TRACK_CHAPTERS)
		s_write(s, "text", 4);
	else
		s_write(s, "soun", 4);

	s_wb32(s, 0); // reserved
	s_wb32(s, 0); // reserved
	s_wb32(s, 0); // reserved

	const char *handler_name;
	if (!track)
		handler_name = "OBS Data Handler";
	else if (track->type == TRACK_VIDEO)
		handler_name = "OBS Video Handler";
	else if (track->type == TRACK_CHAPTERS)
		handler_name = "OBS Chapter Handler";
	else
		handler_name = "OBS Audio Handler";

	// name (null-terminated for MP4, pascal string for MOV)
	size_t handler_len = strlen(handler_name);
	if (mux->flavor == FLAVOR_MOV) {
		s_w8(s, (uint8_t)handler_len);
		s_write(s, handler_name, handler_len);
	} else {
		s_write(s, handler_name, handler_len);
		s_w8(s, 0); // NULL terminator
	}

	return write_box_size(s, start);
}

/// 12.1.2 Video media header
static size_t mp4_write_vmhd(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	/* Flags is always 1 */
	write_fullbox(s, 20, "vmhd", 0, 1);

	s_wb16(s, 0); // graphicsmode
	s_wb16(s, 0); // opcolor r
	s_wb16(s, 0); // opcolor g
	s_wb16(s, 0); // opcolor b

	return 16;
}

/// 12.2.2 Sound media header
static size_t mp4_write_smhd(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	write_fullbox(s, 16, "smhd", 0, 0);

	s_wb16(s, 0); // balance
	s_wb16(s, 0); // reserved

	return 16;
}

/// (QTFF/Apple) Text media information atom
static size_t mp4_write_qt_text(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "text");

	/* Identity matrix, note that it's not fixed point 16.16 */
	s_wb16(s, 0x01);
	s_wb32(s, 0x00);
	s_wb32(s, 0x00);
	s_wb32(s, 0x00);
	s_wb32(s, 0x01);
	s_wb32(s, 0x00);
	s_wb32(s, 0x00);
	s_wb32(s, 0x00);
	s_wb32(s, 0x00004000);
	/* Seemingly undocumented */
	s_wb16(s, 0x0000);

	return write_box_size(s, start);
}

/// (QTFF/Apple) Base media info atom
static size_t mp4_write_gmin(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "gmin", 0, 0);

	s_wb16(s, 0x40);   // graphics mode
	s_wb16(s, 0x8000); // opColor r
	s_wb16(s, 0x8000); // opColor g
	s_wb16(s, 0x8000); // opColor b
	s_wb16(s, 0);      // balance
	s_wb16(s, 0);      // reserved

	return write_box_size(s, start);
}

/// (QTFF/Apple) Base media information header atom
static size_t mp4_write_gmhd(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "gmhd");

	// gmin
	mp4_write_gmin(mux);
	// text (QuickTime)
	mp4_write_qt_text(mux);

	return write_box_size(s, start);
}

/// ISO/IEC 14496-15 5.4.2.1 AVCConfigurationBox
static size_t mp4_write_avcC(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;

	/* For AVC this is the parsed extra data. */
	uint8_t *header;
	size_t size;

	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO, .timebase_den = 1, .keyframe = true};

	if (!obs_encoder_get_extra_data(enc, &header, &size))
		return 0;

	packet.size = obs_parse_avc_header(&packet.data, header, size);

	size_t box_size = packet.size + 8;
	write_box(s, box_size, "avcC");
	s_write(s, packet.data, packet.size);

	bfree(packet.data);
	return box_size;
}

/// ISO/IEC 14496-15 8.4.1.1 HEVCConfigurationBox
static size_t mp4_write_hvcC(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;

	/* For HEVC this is the parsed extra data. */
	uint8_t *header;
	size_t size;

	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO, .timebase_den = 1, .keyframe = true};

	if (!obs_encoder_get_extra_data(enc, &header, &size))
		return 0;

	packet.size = obs_parse_hevc_header(&packet.data, header, size);

	size_t box_size = packet.size + 8;
	write_box(s, box_size, "hvcC");
	s_write(s, packet.data, packet.size);

	bfree(packet.data);
	return box_size;
}

/// AV1 ISOBMFF 2.3. AV1 Codec Configuration Box
static size_t mp4_write_av1C(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;

	/* For AV1 this is just the parsed extra data. */
	uint8_t *header;
	size_t size;

	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO, .timebase_den = 1, .keyframe = true};

	if (!obs_encoder_get_extra_data(enc, &header, &size))
		return 0;

	packet.size = obs_parse_av1_header(&packet.data, header, size);

	size_t box_size = packet.size + 8;
	write_box(s, box_size, "av1C");
	s_write(s, packet.data, packet.size);

	bfree(packet.data);
	return box_size;
}

/// 12.1.5 Colour information
static size_t mp4_write_colr(struct mp4_mux *mux, obs_encoder_t *enc)
{
	UNUSED_PARAMETER(enc);
	struct serializer *s = mux->serializer;

	write_box(s, 19, "colr");

	uint8_t full_range = 0;
	uint16_t pri, trc, spc;
	pri = trc = spc = 0;
	get_colour_information(enc, &pri, &trc, &spc, &full_range);

	s_write(s, "nclx", 4);    // colour_type
	s_wb16(s, pri);           // colour_primaries
	s_wb16(s, trc);           // transfer_characteristics
	s_wb16(s, spc);           // matrix_coefficiencts
	s_w8(s, full_range << 7); // full range flag + 7 reserved bits (0)

	return 19;
}

/// 12.1.4 Pixel Aspect Ratio
static size_t mp4_write_pasp(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	write_box(s, 16, "pasp");

	s_wb32(s, 1); // hSpacing
	s_wb32(s, 1); // vSpacing

	return 16;
}

/// 12.1.3 Visual Sample Entry
static inline void mp4_write_visual_sample_entry(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;

	// SampleEntry Box
	s_w8(s, 0); // reserved
	s_w8(s, 0);
	s_w8(s, 0);
	s_w8(s, 0);
	s_w8(s, 0);
	s_w8(s, 0);

	s_wb16(s, 1); // data_reference_index

	// VisualSampleEntry Box
	s_wb16(s, 0); // pre_defined
	s_wb16(s, 0); // reserved

	if (mux->flavor == FLAVOR_MOV) {
		s_write(s, "OBSS", 4); // vendor
		s_wb32(s, 0x200);      // temporal quality (codecNormalQuality = 512)
		s_wb32(s, 0x200);      // spatial quality (codecNormalQuality)
	} else {
		s_wb32(s, 0); // pre_defined
		s_wb32(s, 0); // pre_defined
		s_wb32(s, 0); // pre_defined
	}

	s_wb16(s, (uint16_t)obs_encoder_get_width(enc));  // width
	s_wb16(s, (uint16_t)obs_encoder_get_height(enc)); // height

	s_wb32(s, 0x00480000); // horizresolution (predefined)
	s_wb32(s, 0x00480000); // vertresolution (predefined)

	s_wb32(s, 0); // reserved
	s_wb16(s, 1); // frame_count

	/* Name is fixed 32-bytes and needs to be padded to that length.
	 * First byte is the length, rest is a string sans NULL terminator. */
	char compressor_name[32] = {0};
	const char *enc_id = obs_encoder_get_id(enc);
	if (enc_id) {
		size_t len = strlen(enc_id);
		if (len > 31)
			len = 31;

		compressor_name[0] = (char)len;
		memcpy(compressor_name + 1, enc_id, len);
	}
	s_write(s, compressor_name, sizeof(compressor_name)); // compressorname

	s_wb16(s, 0x0018); // depth
	s_wb16(s, -1);     // pre_defined
}

/// 12.1.6 Content light level
static size_t mp4_write_clli(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;

	video_t *video = obs_encoder_video(enc);
	const struct video_output_info *info = video_output_get_info(video);

	/* Only write box for HDR video */
	if (info->colorspace != VIDEO_CS_2100_PQ && info->colorspace != VIDEO_CS_2100_HLG)
		return 0;

	write_box(s, 12, "clli");

	float nominal_peak = obs_get_video_hdr_nominal_peak_level();

	s_wb16(s, (uint16_t)nominal_peak); // max_content_light_level
	s_wb16(s, (uint16_t)nominal_peak); // max_pic_average_light_level

	return 12;
}

/// 12.1.7 Mastering display colour volume
static size_t mp4_write_mdcv(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;

	video_t *video = obs_encoder_video(enc);
	const struct video_output_info *info = video_output_get_info(video);

	// Only write atom for HDR video
	if (info->colorspace != VIDEO_CS_2100_PQ && info->colorspace != VIDEO_CS_2100_HLG)
		return 0;

	write_box(s, 32, "mdcv");

	float nominal_peak = obs_get_video_hdr_nominal_peak_level();
	uint32_t max_lum = (uint32_t)nominal_peak * 10000;

	/* Note that these values are hardcoded everywhere in OBS, so these are
	 * just the same as used in our other muxers/encoders. */

	// 3 x display_primaries (x, y) pairs
	s_wb16(s, 13250);
	s_wb16(s, 34500);
	s_wb16(s, 7500);
	s_wb16(s, 3000);
	s_wb16(s, 34000);
	s_wb16(s, 16000);

	s_wb16(s, 15635);   // white_point_x
	s_wb16(s, 16450);   // white_point_y
	s_wb32(s, max_lum); // max_display_mastering_luminance
	s_wb32(s, 0);       // min_display_mastering_luminance

	return 32;
}

/// ISO/IEC 14496-15 5.4.2.1 AVCSampleEntry
static size_t mp4_write_avc1(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "avc1");

	mp4_write_visual_sample_entry(mux, enc);

	// avcC
	mp4_write_avcC(mux, enc);

	// colr
	mp4_write_colr(mux, enc);

	// pasp
	mp4_write_pasp(mux);

	return write_box_size(s, start);
}

/// ISO/IEC 14496-15 8.4.1.1 HEVCSampleEntry
static size_t mp4_write_hvc1(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "hvc1");

	mp4_write_visual_sample_entry(mux, enc);

	// avcC
	mp4_write_hvcC(mux, enc);

	// colr
	mp4_write_colr(mux, enc);

	// clli
	mp4_write_clli(mux, enc);

	// mdcv
	mp4_write_mdcv(mux, enc);

	// pasp
	mp4_write_pasp(mux);

	return write_box_size(s, start);
}

/// AV1 ISOBMFF 2.2. AV1 Sample Entry
static size_t mp4_write_av01(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "av01");

	mp4_write_visual_sample_entry(mux, enc);

	// avcC
	mp4_write_av1C(mux, enc);

	// colr
	mp4_write_colr(mux, enc);

	// clli
	mp4_write_clli(mux, enc);

	// mdcv
	mp4_write_mdcv(mux, enc);

	// pasp
	mp4_write_pasp(mux);

	return write_box_size(s, start);
}

/// (QTFF/Apple) Video Sample Description
static size_t mp4_write_prores(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	/* We get the tag as an int, but need it as a char[4] */
	union tag {
		char c[4];
		uint32_t i;
	} codec_tag;

	/* Codec tag varies for ProRes depending on configuration, so we need to get it from the encoder. */
	obs_data_t *settings = obs_encoder_get_settings(enc);
	codec_tag.i = (uint32_t)obs_data_get_int(settings, "codec_type");
	obs_data_release(settings);

#if __BYTE_ORDER == __LITTLE_ENDIAN
	codec_tag.i = ((codec_tag.i >> 24) & 0x000000FF) | ((codec_tag.i << 8) & 0x00FF0000) |
		      ((codec_tag.i >> 8) & 0x0000FF00) | ((codec_tag.i << 24) & 0xFF000000);
#endif

	write_box(s, 0, codec_tag.c);

	mp4_write_visual_sample_entry(mux, enc);

	// colr
	mp4_write_colr(mux, enc);

	// clli
	mp4_write_clli(mux, enc);

	// mdcv
	mp4_write_mdcv(mux, enc);

	// pasp
	mp4_write_pasp(mux);

	return write_box_size(s, start);
}

static inline void put_descr(struct serializer *s, uint8_t tag, size_t size)
{
	int i = 3;
	s_w8(s, tag);
	for (; i > 0; i--)
		s_w8(s, (uint8_t)((size >> (7 * i)) | 0x80));
	s_w8(s, size & 0x7F);
}

/// ISO/IEC 14496-14 5.6 ESDBox
static size_t mp4_write_esds(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "esds", 0, 0);

	/* Encoder extradata will be used as DecoderSpecificInfo  */
	uint8_t *extradata;
	size_t extradata_size;
	if (!obs_encoder_get_extra_data(track->encoder, &extradata, &extradata_size)) {
		extradata_size = 0;
	}

	/// ISO/IEC 14496-1

	// ES_Descriptor
	size_t decoder_specific_info_len = extradata_size ? extradata_size + 5 : 0;

	put_descr(s, 0x03, 3 + 5 + 13 + decoder_specific_info_len + 5 + 1);
	s_wb16(s, track->track_id);
	s_w8(s, 0x00); // flags

	// DecoderConfigDescriptor
	put_descr(s, 0x04, 13 + decoder_specific_info_len);
	s_w8(s, 0x40); // codec tag, 0x40 = AAC
	s_w8(s, 0x15); // stream type field (0x15 = audio stream)

	/* When writing the final MOOV this could theoretically be calculated
	 * based on chunks, but it's not really all that important. */
	uint32_t bitrate = 0;
	obs_data_t *settings = obs_encoder_get_settings(track->encoder);
	if (settings) {
		int64_t enc_bitrate = obs_data_get_int(settings, "bitrate");
		if (enc_bitrate)
			bitrate = (uint32_t)(enc_bitrate * 1000);

		obs_data_release(settings);
	}

	s_wb24(s, 0);       // bufferSizeDB (in bytes)
	s_wb32(s, bitrate); // maxbitrate
	s_wb32(s, bitrate); // avgBitrate

	// DecoderSpecificInfo
	if (extradata_size) {
		put_descr(s, 0x05, extradata_size);
		s_write(s, extradata, extradata_size);
	}

	// SLConfigDescriptor descriptor
	put_descr(s, 0x06, 1);
	s_w8(s, 0x02); // 0x2 = reserved for MP4, descriptor is empty

	return write_box_size(s, start);
}

/// 12.2.3 Audio Sample Entry
static inline void mp4_write_audio_sample_entry(struct mp4_mux *mux, struct mp4_track *track, uint8_t version)
{
	struct serializer *s = mux->serializer;
	bool is_mov = mux->flavor == FLAVOR_MOV;
	bool is_pcm = track->codec == CODEC_PCM_I16 || track->codec == CODEC_PCM_I24 || track->codec == CODEC_PCM_F32;

	// SampleEntry Box
	s_w8(s, 0); // reserved
	s_w8(s, 0);
	s_w8(s, 0);
	s_w8(s, 0);
	s_w8(s, 0);
	s_w8(s, 0);

	s_wb16(s, 1); // data_reference_index

	// AudioSampleEntry Box
	s_wb16(s, version); // entry_version
	s_wb16(s, 0);       // reserved
	s_wb16(s, 0);       // reserved
	s_wb16(s, 0);       // reserved

	audio_t *audio = obs_encoder_audio(track->encoder);
	uint32_t channels = (uint32_t)audio_output_get_channels(audio);
	uint32_t sample_rate = track->timescale;
	bool alac = track->codec == CODEC_ALAC;

	/* MOV specific version: https://developer.apple.com/documentation/quicktime-file-format/sound_sample_description_version_2 */
	if (version == 2) {
		// We need to get the raw float bytes, union seems to be the easiest way to do that.
		union rate {
			uint64_t u;
			double f;
		} rate;
		rate.f = (double)sample_rate;

		s_wb16(s, 3);          // always3
		s_wb16(s, 16);         // always16
		s_wb16(s, 0xfffe);     // alwaysMinus2
		s_wb16(s, 0);          // always0
		s_wb32(s, 0x00010000); // always65536
		s_wb32(s, 72);         // sizeOfStructOnly (start of containing box to constLPCMFramesPerAudioPacket)
		s_wb64(s, rate.u);     // audioSampleRate
		s_wb32(s, channels);   // numAudioChannels
		s_wb32(s, 0x7F000000); // always7F000000
		s_wb32(s, is_pcm ? track->sample_size / channels * 8 : 0); // constBitsPerChannel
		s_wb32(s, get_lpcm_flags(track->codec));                   // formatSpecificFlags
		s_wb32(s, is_pcm ? track->sample_size : 0);                // constBytesPerAudioPacket
		s_wb32(s, is_pcm ? 1 : 0);                                 // constLPCMFramesPerAudioPacket
	} else {
		s_wb16(s, channels); // channelcount

		/* OBS FLAC is currently always 16-bit, ALAC always 24, this may change in the future and should be
		 * handled differently then.
		 * That being said those codecs are self-describing, so in most cases it shouldn't actually matter. */
		s_wb16(s, !is_mov && alac ? 24 : 16); // samplesize

		s_wb16(s, is_mov && !is_pcm ? -2 : 0); // pre_defined (compression ID in MOV)
		s_wb16(s, 0);                          // reserved

		/* The sample rate field is limited to 16-bits. Technically version 1 supports a "srat" box which
		 * provides 32-bits, but this is not supported by most software (including FFmpeg and Chromium).
		 * For encoded codecs (AAC etc.), the sample rate can be read from the encoded data itself.
		 * For PCM FFmpeg will try to use the timescale as sample rate. */
		if (sample_rate > UINT16_MAX) {
			warn("Sample rate too high for MP4, file may not play back correctly.");
			sample_rate = 0;
		}

		s_wb32(s, sample_rate << 16); // samplerate

		/* MOV-only data: https://developer.apple.com/documentation/quicktime-file-format/sound_sample_description_version_1 */
		if (is_mov && version == 1) {
			size_t frame_size = obs_encoder_get_frame_size(track->encoder);
			s_wb32(s, is_pcm ? 1 : (uint32_t)frame_size);          // frame size
			s_wb32(s, is_pcm ? track->sample_size / channels : 0); // bytes per packet
			s_wb32(s, is_pcm ? track->sample_size : 0);            // bytes per frame
			s_wb32(s, 2); // bytes per sample, 2 for anything but 8-bit
		}
	}
}

/// 12.2.4 Channel layout
static size_t mp4_write_chnl(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "chnl", 0, 0);

	audio_t *audio = obs_encoder_audio(track->encoder);
	const struct audio_output_info *info = audio_output_get_info(audio);

	s_w8(s, 1); // stream_structure (1 = channels)

	/* 5.1 and 4.1 do not have a corresponding ISO layout, so we have to
	 * write a manually created channel map for those. */
	uint8_t map[8] = {0};
	uint8_t items = 0;
	uint8_t defined_layout = 0;

	get_speaker_positions(info->speakers, map, &items, &defined_layout);

	if (!defined_layout) {
		warn("No ISO layout available for speaker layout %d, "
		     "this may not be supported by all applications!",
		     info->speakers);
		s_w8(s, 0);             // definedLayout
		s_write(s, map, items); // uint8_t speaker_position[count]
	} else {
		s_w8(s, defined_layout); // definedLayout
		s_wb64(s, 0);            // ommitedChannelMap
	}

	return write_box_size(s, start);
}

/// ISO/IEC 14496-14 5.6 MP4AudioSampleEntry
static size_t mp4_write_mp4a(struct mp4_mux *mux, struct mp4_track *track, uint8_t version)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "mp4a");

	mp4_write_audio_sample_entry(mux, track, version);

	// esds
	mp4_write_esds(mux, track);

	/* Write channel layout for version 1 sample entires */
	if (version == 1)
		mp4_write_chnl(mux, track);

	return write_box_size(s, start);
}

/// Encapsulation of FLAC in ISO Base Media File Format 3.3.2 FLAC Specific Box
static size_t mp4_write_dfLa(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	uint8_t *extradata;
	size_t extradata_size;

	if (!obs_encoder_get_extra_data(track->encoder, &extradata, &extradata_size))
		return 0;

	write_fullbox(s, 0, "dfLa", 0, 0);

	/// FLACMetadataBlock

	// LastMetadataBlockFlag (1) | BlockType (0)
	s_w8(s, 1 << 7 | 0);
	// Length
	s_wb24(s, (uint32_t)extradata_size);
	// BlockData[Length]
	s_write(s, extradata, extradata_size);

	return write_box_size(s, start);
}

/// Encapsulation of FLAC in ISO Base Media File Format 3.3.1 FLACSampleEntry
static size_t mp4_write_fLaC(struct mp4_mux *mux, struct mp4_track *track, uint8_t version)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "fLaC");

	mp4_write_audio_sample_entry(mux, track, version);

	// dfLa
	mp4_write_dfLa(mux, track);

	if (version == 1)
		mp4_write_chnl(mux, track);

	return write_box_size(s, start);
}

/// Apple Lossless Format "Magic Cookie" Description - MP4/M4A File
static size_t mp4_write_alac(struct mp4_mux *mux, struct mp4_track *track, uint8_t version)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	uint8_t *extradata;
	size_t extradata_size;

	if (!obs_encoder_get_extra_data(track->encoder, &extradata, &extradata_size))
		return 0;

	write_box(s, 0, "alac");

	mp4_write_audio_sample_entry(mux, track, version);

	/* Apple Lossless Magic Cookie */
	s_write(s, extradata, extradata_size);

	if (version == 1)
		mp4_write_chnl(mux, track);

	return write_box_size(s, start);
}

/// ISO/IEC 23003-5 5.1 PCM configuration
static size_t mp4_write_pcmc(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "pcmC", 0, 0);

	s_w8(s, 1); // endianness, 1 = little endian

	// bits per sample
	if (track->codec == CODEC_PCM_I16)
		s_w8(s, 16);
	else if (track->codec == CODEC_PCM_I24)
		s_w8(s, 24);
	else if (track->codec == CODEC_PCM_F32)
		s_w8(s, 32);

	return write_box_size(s, start);
}

/// ISO/IEC 23003-5 5.1 PCM configuration
static size_t mp4_write_xpcm(struct mp4_mux *mux, struct mp4_track *track, uint8_t version)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	/* Different box types for floating point and integer PCM*/
	write_box(s, 0, track->codec == CODEC_PCM_F32 ? "fpcm" : "ipcm");

	mp4_write_audio_sample_entry(mux, track, version);

	/* ChannelLayout (chnl) is required for PCM */
	mp4_write_chnl(mux, track);

	// pcmc
	mp4_write_pcmc(mux, track);

	return write_box_size(s, start);
}

/// (QTFF/Apple) Text sample description
static size_t mp4_write_text(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "text", 0, 0);

	s_wb32(s, 1); // number of entries

	/* Preset sample description as used by FFmpeg. */
	s_write(s, &TEXT_STUB_HEADER, sizeof(TEXT_STUB_HEADER));

	return write_box_size(s, start);
}

static inline uint32_t rl32(const uint8_t *ptr)
{
	return (ptr[3] << 24) + (ptr[2] << 16) + (ptr[1] << 8) + ptr[0];
}

static inline uint16_t rl16(const uint8_t *ptr)
{
	return (ptr[1] << 8) + ptr[0];
}

/// Encapsulation of Opus in ISO Base Media File Format 4.3.2 Opus Specific Box
static size_t mp4_write_dOps(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	uint8_t *extradata;
	size_t extradata_size;

	if (!obs_encoder_get_extra_data(track->encoder, &extradata, &extradata_size))
		return 0;

	write_box(s, 0, "dOps");
	s_w8(s, 0); // version

	uint8_t channels = *(extradata + 9);
	uint8_t channel_map = *(extradata + 18);

	s_w8(s, channels); // channel count
	// OpusHead is little-endian, but MP4 is big-endian, so we have to swap them here
	s_wb16(s, rl16(extradata + 10)); // pre-skip
	s_wb32(s, rl32(extradata + 12)); // input sample rate
	s_wb16(s, rl16(extradata + 16)); // output gain
	s_w8(s, channel_map);            // channel mapping family

	if (channel_map)
		s_write(s, extradata + 19, 2 + channels);

	return write_box_size(s, start);
}

/// Encapsulation of Opus in ISO Base Media File Format 4.3.1 Sample entry format
static size_t mp4_write_Opus(struct mp4_mux *mux, struct mp4_track *track, uint8_t version)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "Opus");

	mp4_write_audio_sample_entry(mux, track, version);

	// dOps
	mp4_write_dOps(mux, track);

	if (version == 1)
		mp4_write_chnl(mux, track);

	return write_box_size(s, start);
}

/// (QTFF/Apple) siDecompressionParam Atom ('wave')
static size_t mp4_write_wave(struct mp4_mux *mux, struct mp4_track *track, const char tag[4])
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "wave");

	/* frma atom containing codec tag (again) */
	s_wb32(s, 12);
	s_write(s, "frma", 4);
	s_write(s, tag, 4);

	if (track->codec == CODEC_AAC) {
		mp4_write_esds(mux, track);
	} else if (track->codec == CODEC_ALAC) {
		uint8_t *extradata;
		size_t extradata_size;

		if (obs_encoder_get_extra_data(track->encoder, &extradata, &extradata_size)) {
			/* Apple Lossless Magic Cookie */
			s_write(s, extradata, extradata_size);
		}
	}

	/* Terminator atom */
	s_wb32(s, 8); // size
	s_wb32(s, 0); // NULL name

	return write_box_size(s, start);
}

/// (QTFF/Apple) Audio Channel Layout Atom (‘chan’)
static size_t mp4_write_chan(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	audio_t *audio = obs_encoder_audio(track->encoder);
	const struct audio_output_info *info = audio_output_get_info(audio);
	uint32_t layout = get_mov_channel_layout(track->codec, info->speakers);
	uint32_t bitmap = layout == kAudioChannelLayoutTag_UseChannelBitmap ? get_mov_channel_bitmap(info->speakers)
									    : 0;
	if (layout == kAudioChannelLayoutTag_UseChannelBitmap && !bitmap) {
		warn("No valid speaker layout found, not writing chan box. File may not play back correctly!");
		return 0;
	}

	write_fullbox(s, 0, "chan", 0, 0);
	/* AudioChannelLayout from CoreAudioTypes.h */
	s_wb32(s, layout); // mChannelLayoutTag
	s_wb32(s, bitmap); // mChannelBitmap
	s_wb32(s, 0);      // mNumberChannelDescriptions

	return write_box_size(s, start);
}

/// (QTFF/Apple) Sound Sample Description (v1 and v2)
static size_t mp4_write_mov_audio_tag(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	const char *tag = NULL;
	audio_t *audio = obs_encoder_audio(track->encoder);
	uint32_t sample_rate = audio_output_get_sample_rate(audio);
	size_t channels = audio_output_get_channels(audio);
	/* More than 2 channels or samples rates above 65535 Hz requires v2 */
	uint8_t version = (channels > 2 || sample_rate > UINT16_MAX) ? 2 : 1;

	if (track->codec == CODEC_PCM_F32 || track->codec == CODEC_PCM_I16 || track->codec == CODEC_PCM_I24) {
		tag = "lpcm";
		version = 2; /* lpcm also requires v2 */
	} else if (track->codec == CODEC_AAC) {
		tag = "mp4a";
	} else if (track->codec == CODEC_ALAC) {
		tag = "alac";
	}

	/* Unsupported/Unknown codec */
	if (!tag)
		return 0;

	write_box(s, 0, tag);

	mp4_write_audio_sample_entry(mux, track, version);

	// wave
	if (version == 1)
		mp4_write_wave(mux, track, tag);

	// chan
	mp4_write_chan(mux, track);

	return write_box_size(s, start);
}

/// 8.5.2 Sample Description Box
static size_t mp4_write_stsd(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	/* Anything but mono or stereo technically requires v1,
	 * but in practice that doesn't appear to matter. */
	uint8_t version = 0;

	if (track->type == TRACK_AUDIO && mux->flavor != FLAVOR_MOV) {
		audio_t *audio = obs_encoder_audio(track->encoder);
		version = audio_output_get_channels(audio) > 2 ? 1 : 0;
	}

	write_fullbox(s, 0, "stsd", version, 0);

	s_wb32(s, 1); // entry_count

	// codec specific boxes
	if (track->type == TRACK_VIDEO) {
		if (track->codec == CODEC_H264)
			mp4_write_avc1(mux, track->encoder);
		else if (track->codec == CODEC_HEVC)
			mp4_write_hvc1(mux, track->encoder);
		else if (track->codec == CODEC_AV1)
			mp4_write_av01(mux, track->encoder);
		else if (track->codec == CODEC_PRORES)
			mp4_write_prores(mux, track->encoder);
	} else if (track->type == TRACK_AUDIO) {
		if (mux->flavor == FLAVOR_MOV) {
			mp4_write_mov_audio_tag(mux, track);
		} else {
			if (track->codec == CODEC_AAC)
				mp4_write_mp4a(mux, track, version);
			else if (track->codec == CODEC_OPUS)
				mp4_write_Opus(mux, track, version);
			else if (track->codec == CODEC_FLAC)
				mp4_write_fLaC(mux, track, version);
			else if (track->codec == CODEC_ALAC)
				mp4_write_alac(mux, track, version);
			else if (track->codec == CODEC_PCM_I16 || track->codec == CODEC_PCM_I24 ||
				 track->codec == CODEC_PCM_F32)
				mp4_write_xpcm(mux, track, version);
		}
	} else if (track->type == TRACK_CHAPTERS) {
		mp4_write_text(mux);
	}

	return write_box_size(s, start);
}

/// 8.6.1.2 Decoding Time to Sample Box
static size_t mp4_write_stts(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;

	if (fragmented) {
		write_fullbox(s, 16, "stts", 0, 0);
		s_wb32(s, 0); // entry_count
		return 16;
	}

	int64_t start = serializer_get_pos(s);
	struct sample_delta *arr = track->deltas.array;
	size_t num = track->deltas.num;

	write_fullbox(s, 0, "stts", 0, 0);

	s_wb32(s, (uint32_t)num); // entry_count

	for (size_t idx = 0; idx < num; idx++) {
		struct sample_delta *smp = &arr[idx];

		uint64_t delta = util_mul_div64(smp->delta, track->timescale, track->timebase_den);

		s_wb32(s, smp->count);      // sample_count
		s_wb32(s, (uint32_t)delta); // sample_delta
	}

	return write_box_size(s, start);
}

/// 8.6.2 Sync Sample Box
static size_t mp4_write_stss(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	uint32_t num = (uint32_t)track->sync_samples.num;

	if (!num)
		return 0;

	/* 16 byte FullBox header + 4-bytes (u32) per sync sample */
	uint32_t size = 16 + 4 * num;

	write_fullbox(s, size, "stss", 0, 0);
	s_wb32(s, num); // entry_count

	for (size_t idx = 0; idx < num; idx++)
		s_wb32(s, track->sync_samples.array[idx]); // sample_number

	return size;
}

/// 8.6.1.3 Composition Time to Sample Box
static size_t mp4_write_ctts(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	uint32_t num = (uint32_t)track->offsets.num;

	uint8_t version = mux->flags & MP4_USE_NEGATIVE_CTS ? 1 : 0;

	/* 16 byte FullBox header + 8-bytes (u32+u32/i32) per offset entry */
	uint32_t size = 16 + 8 * num;
	write_fullbox(s, size, "ctts", version, 0);

	s_wb32(s, num); // entry_count

	for (size_t idx = 0; idx < num; idx++) {
		int64_t offset = (int64_t)track->offsets.array[idx].offset * (int64_t)track->timescale /
				 (int64_t)track->timebase_den;

		s_wb32(s, track->offsets.array[idx].count); // sample_count
		s_wb32(s, (uint32_t)offset);                // sample_offset
	}

	return size;
}

/// 8.7.4 Sample To Chunk Box
static size_t mp4_write_stsc(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;

	if (fragmented) {
		write_fullbox(s, 16, "stsc", 0, 0);
		s_wb32(s, 0); // entry_count
		return 16;
	}

	struct chunk *arr = track->chunks.array;
	size_t arr_num = track->chunks.num;

	/* Compress into array with counter for repeating chunk sizes */
	DARRAY(struct chunk_run {
		uint32_t first;
		uint32_t samples;
	}) chunk_runs;

	da_init(chunk_runs);

	for (size_t idx = 0; idx < arr_num; idx++) {
		struct chunk *chk = &arr[idx];

		if (!chunk_runs.num || chunk_runs.array[chunk_runs.num - 1].samples != chk->samples) {
			struct chunk_run *cr = da_push_back_new(chunk_runs);
			cr->samples = chk->samples;
			cr->first = (uint32_t)idx + 1; // ISO-BMFF is 1-indexed
		}
	}

	uint32_t num = (uint32_t)chunk_runs.num;

	/* 16 byte FullBox header + 12-bytes (u32+u32+u32) per chunk run */
	uint32_t size = 16 + 12 * num;
	write_fullbox(s, size, "stsc", 0, 0);

	s_wb32(s, num); // entry_count

	for (size_t idx = 0; idx < num; idx++) {
		struct chunk_run *cr = &chunk_runs.array[idx];
		s_wb32(s, cr->first);   // first_chunk
		s_wb32(s, cr->samples); // samples_per_chunk
		s_wb32(s, 1);           // sample_description_index
	}

	da_free(chunk_runs);

	return size;
}

/// 8.7.3 Sample Size Boxes
static size_t mp4_write_stsz(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;

	if (fragmented) {
		write_fullbox(s, 20, "stsz", 0, 0);
		s_wb32(s, 0); // sample_size
		s_wb32(s, 0); // sample_count

		return 20;
	}

	int64_t start = serializer_get_pos(s);

	/* This should only ever happen when recording > 24 hours of
	 * 48 kHz PCM audio or 828 days of 60 FPS video. */
	if (track->samples > UINT32_MAX) {
		warn("Track %u has too many samples, its duration may not be "
		     "read correctly. Remuxing the file to another format such "
		     "as MKV may be required.",
		     track->track_id);
	}

	write_fullbox(s, 0, "stsz", 0, 0);

	if (track->sample_size) {
		/* Fixed size samples mean we don't need an array */
		s_wb32(s, track->sample_size);       // sample_size
		s_wb32(s, (uint32_t)track->samples); // sample_count
	} else {
		s_wb32(s, 0);                                 // sample_size
		s_wb32(s, (uint32_t)track->sample_sizes.num); // sample_count

		for (size_t idx = 0; idx < track->sample_sizes.num; idx++) {
			s_wb32(s, track->sample_sizes.array[idx]); // entry_size
		}
	}

	return write_box_size(s, start);
}

/// 8.7.5 Chunk Offset Box
static size_t mp4_write_stco(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;

	if (fragmented) {
		write_fullbox(s, 16, "stco", 0, 0);
		s_wb32(s, 0); // entry_count
		return 16;
	}

	struct chunk *arr = track->chunks.array;
	uint32_t num = (uint32_t)track->chunks.num;

	uint64_t last_off = arr[num - 1].offset;
	uint32_t size;
	bool co64 = last_off > UINT32_MAX;

	/* When using 64-bit offsets we write 8-bytes (u64) per chunk,
	 * otherwise 4-bytes (u32). */
	if (co64) {
		size = 16 + 8 * num;
		write_fullbox(s, size, "co64", 0, 0);
	} else {
		size = 16 + 4 * num;
		write_fullbox(s, size, "stco", 0, 0);
	}

	s_wb32(s, num); // entry_count

	for (size_t idx = 0; idx < num; idx++) {
		if (co64)
			s_wb64(s, arr[idx].offset); // chunk_offset
		else
			s_wb32(s, (uint32_t)arr[idx].offset); // chunk_offset
	}

	return size;
}

/// 8.9.3 Sample Group Description Box
static size_t mp4_write_sgpd_aac(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	int64_t start = serializer_get_pos(s);
	write_fullbox(s, 0, "sgpd", 1, 0);

	s_write(s, "roll", 4); // grouping_tpye
	s_wb32(s, 2);          // default_length (i16)

	s_wb32(s, 1); // entry_count

	// AudioRollRecoveryEntry
	s_wb16(s, -1); // roll_distance

	return write_box_size(s, start);
}

/// 8.9.2 Sample to Group Box
static size_t mp4_write_sbgp_aac(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;

	int64_t start = serializer_get_pos(s);
	write_fullbox(s, 0, "sbgp", 0, 0);

	/// 10.1 AudioRollRecoveryEntry
	s_write(s, "roll", 4); // grouping_tpye

	s_wb32(s, 1); // entry_count

	s_wb32(s, (uint32_t)track->samples); // sample_count
	s_wb32(s, 1);                        // group_description_index

	return write_box_size(s, start);
}

static size_t mp4_write_sbgp_sbgp_opus(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	/// 8.9.3 Sample Group Description Box
	write_fullbox(s, 0, "sgpd", 1, 0);

	s_write(s, "roll", 4); // grouping_tpye
	s_wb32(s, 2);          // default_length (i16)

	/* Opus requires 80 ms of preroll, which at 48 kHz is 3840 PCM samples */
	const int64_t opus_preroll = 3840;

	/* Compute the preroll samples (should be 4, each being 20 ms) */
	uint16_t preroll_count = 0;
	int64_t preroll_remaining = opus_preroll;

	for (size_t i = 0; i < track->deltas.num && preroll_remaining > 0; i++) {
		for (uint32_t j = 0; j < track->deltas.array[i].count && preroll_remaining > 0; j++) {
			preroll_remaining -= track->deltas.array[i].delta;
			preroll_count++;
		}
	}

	s_wb32(s, 1); // entry_count
	/// 10.1 AudioRollRecoveryEntry
	s_wb16(s, -preroll_count); // roll_distance

	size_t size_sgpd = write_box_size(s, start);

	/* --------------- */

	/// 8.9.2 Sample to Group Box
	start = serializer_get_pos(s);
	write_fullbox(s, 0, "sbgp", 0, 0);

	s_write(s, "roll", 4); // grouping_tpye
	s_wb32(s, 2);          // entry_count

	// entry 0
	s_wb32(s, preroll_count); // sample_count
	s_wb32(s, 0);             // group_description_index
	// entry 1
	s_wb32(s, (uint32_t)track->samples - preroll_count); // sample_count
	s_wb32(s, 1);                                        // group_description_index

	return size_sgpd + write_box_size(s, start);
}

/// 8.5.1 Sample Table Box
static size_t mp4_write_stbl(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;

	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "stbl");

	// stsd
	mp4_write_stsd(mux, track);

	// stts
	mp4_write_stts(mux, track, fragmented);

	// stss (non-fragmented/non-prores only)
	if (track->type == TRACK_VIDEO && !fragmented && track->codec != CODEC_PRORES)
		mp4_write_stss(mux, track);

	// ctts (non-fragmented only)
	if (track->needs_ctts && !fragmented)
		mp4_write_ctts(mux, track);

	// stsc
	mp4_write_stsc(mux, track, fragmented);

	// stsz
	mp4_write_stsz(mux, track, fragmented);

	// stco
	mp4_write_stco(mux, track, fragmented);

	if (!fragmented) {
		/* AAC and Opus require a pre-roll to get correct decoder
		 * output, sgpd and sbgp are used to create a "roll" group. */
		if (track->codec == CODEC_AAC) {
			// sgpd
			mp4_write_sgpd_aac(mux);
			// sbgp
			mp4_write_sbgp_aac(mux, track);
		} else if (track->codec == CODEC_OPUS) {
			// sgpd + sbgp
			mp4_write_sbgp_sbgp_opus(mux, track);
		}
	}

	return write_box_size(s, start);
}

/// 8.7.2.2 DataEntryUrlBox
static size_t mp4_write_url(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "url ", 0, 1);

	/* empty, flag 1 means data is in this file */

	return write_box_size(s, start);
}

/// 8.7.2 Data Reference Box
static size_t mp4_write_dref(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "dref ", 0, 0);

	s_wb32(s, 1); // entry_count

	mp4_write_url(mux);

	return write_box_size(s, start);
}

/// 8.7.1 Data Information Box
static size_t mp4_write_dinf(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "dinf");

	mp4_write_dref(mux);

	return write_box_size(s, start);
}

/// 8.4.4 Media Information Box
static size_t mp4_write_minf(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "minf");

	// vmhd/smhd/gmhd
	if (track->type == TRACK_VIDEO)
		mp4_write_vmhd(mux);
	else if (track->type == TRACK_CHAPTERS)
		mp4_write_gmhd(mux);
	else
		mp4_write_smhd(mux);

	// hdlr for dinf, required in MOV only
	if (mux->flavor == FLAVOR_MOV)
		mp4_write_hdlr(mux, NULL);

	// dinf, unnecessary but mandatory
	mp4_write_dinf(mux);

	// stbl
	mp4_write_stbl(mux, track, fragmented);

	return write_box_size(s, start);
}

/// 8.4.1 Media Box
static size_t mp4_write_mdia(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "mdia");

	// mdhd
	mp4_write_mdhd(mux, track);

	// hdlr
	mp4_write_hdlr(mux, track);

	// minf
	mp4_write_minf(mux, track, fragmented);

	return write_box_size(s, start);
}

/// (QTFF/Apple) User data atom
static size_t mp4_write_udta_atom(struct mp4_mux *mux, const char tag[4], const char *val)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, tag);
	s_write(s, val, strlen(val));

	return write_box_size(s, start);
}

/// 8.10.1 User Data Box
static size_t mp4_write_track_udta(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "udta");

	/* Our udta box contains QuickTime format user data atoms, which are
	 * simple key-value pairs. Some are prefixed with 0xa9. */

	const char *name = obs_encoder_get_name(track->encoder);
	if (name)
		mp4_write_udta_atom(mux, "name", name);

	if (mux->flags & MP4_WRITE_ENCODER_INFO) {
		const char *id = obs_encoder_get_id(track->encoder);
		if (name)
			mp4_write_udta_atom(mux, "\251enc", id);

		obs_data_t *settings = obs_encoder_get_settings(track->encoder);
		if (settings) {
			const char *json = obs_data_get_json_with_defaults(settings);
			mp4_write_udta_atom(mux, "json", json);
			obs_data_release(settings);
		}
	}

	return write_box_size(s, start);
}

/// 8.6.6 Edit List Box
static size_t mp4_write_elst(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "elst", 0, 0);

	s_wb32(s, 1); // entry count

	uint64_t duration = util_mul_div64(track->duration, 1000, track->timebase_den);
	uint64_t delay = 0;

	if (track->type == TRACK_VIDEO && !(mux->flags & MP4_USE_NEGATIVE_CTS)) {
		/* Compensate for frame-reordering delay (for example, when
		 * using b-frames). */
		int64_t dts_offset = 0;

		if (track->offsets.num) {
			struct sample_offset sample = track->offsets.array[0];
			dts_offset = sample.offset;
		} else if (track->packets.size) {
			/* If no offset data exists yet (i.e. when writing the
			 * incomplete moov in a fragmented file) use the raw
			 * data from the current queued packets instead. */
			struct encoder_packet pkt;
			deque_peek_front(&track->packets, &pkt, sizeof(pkt));
			dts_offset = pkt.pts - pkt.dts;
		}

		delay = util_mul_div64(dts_offset, track->timescale, track->timebase_den);
	} else if (track->type == TRACK_AUDIO && track->first_pts < 0) {
		delay = util_mul_div64(llabs(track->first_pts), track->timescale, track->timebase_den);
		/* Subtract priming delay from total duration */
		duration -= util_mul_div64(delay, 1000, track->timescale);
	}

	s_wb32(s, (uint32_t)duration); // segment_duration (movie timescale)
	s_wb32(s, (uint32_t)delay);    // media_time (track timescale)
	s_wb32(s, 1 << 16);            // media_rate

	return write_box_size(s, start);
}

/// 8.6.5 Edit Box
static size_t mp4_write_edts(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "edts");

	mp4_write_elst(mux, track);

	return write_box_size(s, start);
}

/// 8.3.3.2 TrackReferenceTypeBox
static size_t mp4_write_chap(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	/// QTFF/Apple chapter track reference
	write_box(s, 0, "chap");

	s_wb32(s, mux->chapter_track->track_id);

	return write_box_size(s, start);
}

/// 8.3.3 Track Reference Box
static size_t mp4_write_tref(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "tref");

	mp4_write_chap(mux);

	return write_box_size(s, start);
}

/// 8.3.1 Track Box
static size_t mp4_write_trak(struct mp4_mux *mux, struct mp4_track *track, bool fragmented)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	/* If track has no data, omit it from full moov. */
	if (!fragmented && !track->chunks.num)
		return 0;

	write_box(s, 0, "trak");

	// tkhd
	mp4_write_tkhd(mux, track);

	// edts
	mp4_write_edts(mux, track);

	// tref
	if (mux->chapter_track && track->type != TRACK_CHAPTERS)
		mp4_write_tref(mux);

	// mdia
	mp4_write_mdia(mux, track, fragmented);

	// udta (audio track name mainly)
	mp4_write_track_udta(mux, track);

	return write_box_size(s, start);
}

/// 8.8.3 Track Extends Box
static size_t mp4_write_trex(struct mp4_mux *mux, uint32_t track_id)
{
	struct serializer *s = mux->serializer;

	write_fullbox(s, 32, "trex", 0, 0);

	s_wb32(s, track_id); // track_ID
	s_wb32(s, 1);        // default_sample_description_index
	s_wb32(s, 0);        // default_sample_duration
	s_wb32(s, 0);        // default_sample_size
	s_wb32(s, 0);        // default_sample_flags

	return 32;
}

/// 8.8.1 Movie Extends Box
static size_t mp4_write_mvex(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "mvex");

	for (size_t track_id = 0; track_id < mux->tracks.num; track_id++)
		mp4_write_trex(mux, (uint32_t)(track_id + 1));

	return write_box_size(s, start);
}

/// (QTFF/Apple) Undocumented QuickTime/iTunes metadata handler
static size_t mp4_write_itunes_hdlr(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	write_fullbox(s, 33, "hdlr", 0, 0);

	s_wb32(s, 0);          // pre_defined
	s_write(s, "mdir", 4); // handler_type

	// reserved
	s_write(s, "appl", 4);
	s_wb32(s, 0);
	s_wb32(s, 0);

	s_w8(s, 0); // name (NULL)

	return 33;
}

/// (QTFF/Apple) Data atom
static size_t mp4_write_data_atom(struct mp4_mux *mux, const char *data)
{
	struct serializer *s = mux->serializer;

	size_t len = strlen(data);
	uint32_t size = 16 + (uint32_t)len;

	write_box(s, size, "data");

	s_wb32(s, 1); // type, 1 = utf-8 string
	s_wb32(s, 0); // locale, 0 = default
	s_write(s, data, len);

	return size;
}

/// (QTFF/Apple) String atom
static size_t mp4_write_string_data_atom(struct mp4_mux *mux, const char name[4], const char *data)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	uint16_t len = (uint16_t)strlen(data);

	write_box(s, 0, name);
	s_wb16(s, len);            // String length
	s_write(s, "\x55\xC4", 2); // language code, just using undefined
	s_write(s, data, len);     // Note: No NULL terminator

	return write_box_size(s, start);
}

/// (QTFF/Apple) Metadata item atom
static size_t mp4_write_ilst_item_atom(struct mp4_mux *mux, const char name[4], const char *value)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, name);

	mp4_write_data_atom(mux, value);

	return write_box_size(s, start);
}

/// (QTFF/Apple) Metadata item list atom
static size_t mp4_write_ilst(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	struct dstr value = {0};
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "ilst");

	/* Encoder name */
	dstr_cat(&value, "OBS Studio (");
	dstr_cat(&value, obs_get_version_string());
	dstr_cat(&value, ")");
	/* Some QuickTime keys are prefixed with 0xa9 */
	mp4_write_ilst_item_atom(mux, "\251too", value.array);

	dstr_free(&value);

	return write_box_size(s, start);
}

/// (QTFF/Apple) Key value metadata handler
static size_t mp4_write_mdta_hdlr(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	write_fullbox(s, 33, "hdlr", 0, 0);

	s_wb32(s, 0);          // pre_defined
	s_write(s, "mdta", 4); // handler_type

	// reserved
	s_wb32(s, 0);
	s_wb32(s, 0);
	s_wb32(s, 0);

	s_w8(s, 0); // name (NULL)
	return 33;
}

/// (QTFF/Apple) Metadata item keys atom
static size_t mp4_write_mdta_keys(struct mp4_mux *mux, obs_data_t *meta)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "keys", 0, 0);

	uint32_t count = 0;
	int64_t count_pos = serializer_get_pos(s);
	s_wb32(s, count); // count

	obs_data_item_t *item = obs_data_first(meta);

	for (; item != NULL; obs_data_item_next(&item)) {
		const char *name = obs_data_item_get_name(item);
		size_t len = strlen(name);

		/* name is key type, can be udta or mdta */
		write_box(s, len + 8, "mdta");
		s_write(s, name, len); // key name

		count++;
	}

	int64_t end = serializer_get_pos(s);

	/* Overwrite count with correct value */
	serializer_seek(s, count_pos, SERIALIZE_SEEK_START);
	s_wb32(s, count);
	serializer_seek(s, end, SERIALIZE_SEEK_START);

	return write_box_size(s, start);
}

/// (QTFF/Apple) Metadata item atom, but name is an index instead
static inline void write_key_entry(struct mp4_mux *mux, obs_data_item_t *item, uint32_t idx)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	s_wb32(s, 0);   // size
	s_wb32(s, idx); // index

	mp4_write_data_atom(mux, obs_data_item_get_string(item));

	write_box_size(s, start);
}

/// (QTFF/Apple) Metadata item list atom
static size_t mp4_write_mdta_ilst(struct mp4_mux *mux, obs_data_t *meta)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "ilst");

	/* indices start with 1 */
	uint32_t key_idx = 1;

	obs_data_item_t *item = obs_data_first(meta);

	for (; item != NULL; obs_data_item_next(&item)) {
		write_key_entry(mux, item, key_idx);
		key_idx++;
	}

	return write_box_size(s, start);
}

static void mp4_write_mdta_kv(struct mp4_mux *mux)
{
	struct dstr value = {0};

	obs_data_t *meta = obs_data_create();

	dstr_cat(&value, "OBS Studio (");
	dstr_cat(&value, obs_get_version_string());
	dstr_cat(&value, ")");

	// ToDo figure out what else we could put in here for fun and profit :)
	obs_data_set_string(meta, "tool", value.array);

	/* Write keys */
	mp4_write_mdta_keys(mux, meta);
	/* Write values */
	mp4_write_mdta_ilst(mux, meta);

	obs_data_release(meta);
	dstr_free(&value);
}

/// 8.11.1 The Meta box
static size_t mp4_write_meta(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_fullbox(s, 0, "meta", 0, 0);

	if (mux->flags & MP4_USE_MDTA_KEY_VALUE) {
		mp4_write_mdta_hdlr(mux);
		mp4_write_mdta_kv(mux);
	} else {
		mp4_write_itunes_hdlr(mux);
		mp4_write_ilst(mux);
	}

	return write_box_size(s, start);
}

/// 8.10.1 User Data Box
static size_t mp4_write_udta(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "udta");

	/* Normally metadata would be directly in the moov, but since this is
	 * Apple/QTFF format metadata it is inside udta. */

	if (mux->flavor == FLAVOR_MOV && !(mux->flags & MP4_USE_MDTA_KEY_VALUE)) {
		// keys directly in udta atom
		struct dstr value = {0};

		/* Encoder name */
		dstr_cat(&value, "OBS Studio (");
		dstr_cat(&value, obs_get_version_string());
		dstr_cat(&value, ")");
		mp4_write_string_data_atom(mux, "\251swr", value.array);

		dstr_free(&value);
	} else {
		// meta
		mp4_write_meta(mux);
	}

	return write_box_size(s, start);
}

/// Movie Box (8.2.1)
static size_t mp4_write_moov(struct mp4_mux *mux, bool fragmented)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "moov");

	mp4_write_mvhd(mux);

	// trak(s)
	for (size_t i = 0; i < mux->tracks.num; i++) {
		struct mp4_track *track = &mux->tracks.array[i];
		mp4_write_trak(mux, track, fragmented);
	}

	if (!fragmented && mux->chapter_track)
		mp4_write_trak(mux, mux->chapter_track, false);

	// mvex
	if (fragmented)
		mp4_write_mvex(mux);

	// udta (metadata)
	mp4_write_udta(mux);

	return write_box_size(s, start);
}

/* ========================================================================== */
/* moof (fragment header) stuff                                               */

/// 8.8.5 Movie Fragment Header Box
static size_t mp4_write_mfhd(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	write_fullbox(s, 16, "mfhd", 0, 0);

	s_wb32(s, mux->fragments_written); // sequence_number

	return 16;
}

/// 8.8.7 Track Fragment Header Box
static size_t mp4_write_tfhd(struct mp4_mux *mux, struct mp4_track *track, size_t moof_start)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	uint32_t flags = BASE_DATA_OFFSET_PRESENT | DEFAULT_SAMPLE_FLAGS_PRESENT;

	/* Add default size/duration if all samples match. */
	bool durations_match = true;
	bool sizes_match = true;
	uint32_t duration;
	uint32_t sample_size;

	if (track->sample_size) {
		duration = 1;
		sample_size = track->sample_size;
	} else {
		duration = track->fragment_samples.array[0].duration;
		sample_size = track->fragment_samples.array[0].size;

		for (size_t idx = 1; idx < track->fragment_samples.num; idx++) {
			uint32_t frag_duration = track->fragment_samples.array[idx].duration;
			uint32_t frag_size = track->fragment_samples.array[idx].size;

			durations_match = frag_duration == duration;
			sizes_match = frag_size == sample_size;
		}
	}

	if (durations_match)
		flags |= DEFAULT_SAMPLE_DURATION_PRESENT;
	if (sizes_match)
		flags |= DEFAULT_SAMPLE_SIZE_PRESENT;

	write_fullbox(s, 0, "tfhd", 0, flags);

	s_wb32(s, track->track_id); // track_ID
	s_wb64(s, moof_start);      // base_data_offset

	// default_sample_duration
	if (durations_match) {
		if (track->type == TRACK_VIDEO) {
			/* Convert duration to track timescale */
			duration = (uint32_t)util_mul_div64(duration, track->timescale, track->timebase_den);
		}

		s_wb32(s, duration);
	}
	// default_sample_size
	if (sizes_match)
		s_wb32(s, sample_size);
	// default_sample_flags
	if (track->type == TRACK_VIDEO) {
		s_wb32(s, SAMPLE_FLAG_DEPENDS_YES | SAMPLE_FLAG_IS_NON_SYNC);
	} else {
		s_wb32(s, SAMPLE_FLAG_DEPENDS_NO);
	}

	return write_box_size(s, start);
}

/// 8.8.12 Track fragment decode time
static size_t mp4_write_tfdt(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;

	write_fullbox(s, 20, "tfdt", 1, 0);

	/* Subtract samples that are not written yet */
	uint64_t duration_written = track->duration;
	for (size_t i = 0; i < track->fragment_samples.num; i++)
		duration_written -= track->fragment_samples.array[i].duration;

	if (track->type == TRACK_VIDEO) {
		/* Convert to track timescale */
		duration_written = util_mul_div64(duration_written, track->timescale, track->timebase_den);
	}

	s_wb64(s, duration_written); // baseMediaDecodeTime

	return 20;
}

/// 8.8.8 Track Fragment Run Box
static size_t mp4_write_trun(struct mp4_mux *mux, struct mp4_track *track, uint32_t moof_size,
			     uint64_t *samples_mdat_offset)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	uint32_t flags = DATA_OFFSET_PRESENT;

	if (!track->sample_size)
		flags |= SAMPLE_SIZE_PRESENT;

	if (track->type == TRACK_VIDEO) {
		flags |= FIRST_SAMPLE_FLAGS_PRESENT;
		flags |= SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT;
	}

	uint8_t version = mux->flags & MP4_USE_NEGATIVE_CTS ? 1 : 0;

	write_fullbox(s, 0, "trun", version, flags);

	/* moof_size + 8 bytes for mdat header + offset into mdat box data */
	size_t data_offset = moof_size + 8 + *samples_mdat_offset;
	size_t sample_count = track->fragment_samples.num;

	if (track->sample_size) {
		/* Update count based on fixed size */
		size_t total_size = 0;
		for (size_t i = 0; i < sample_count; i++)
			total_size += track->fragment_samples.array[i].size;

		*samples_mdat_offset += total_size;
		sample_count = total_size / track->sample_size;
	}

	s_wb32(s, (uint32_t)sample_count); // sample_count
	s_wb32(s, (uint32_t)data_offset);  // data_offset

	/* If we have a fixed sample size (PCM audio) we only need to write
	 * the sample count and offset. */
	if (track->sample_size)
		return write_box_size(s, start);

	if (track->type == TRACK_VIDEO)
		s_wb32(s, SAMPLE_FLAG_DEPENDS_NO); // first_sample_flags

	for (size_t idx = 0; idx < sample_count; idx++) {
		struct fragment_sample *smp = &track->fragment_samples.array[idx];

		s_wb32(s, smp->size); // sample_size

		if (track->type == TRACK_VIDEO) {
			// sample_composition_time_offset
			int64_t offset =
				(int64_t)smp->offset * (int64_t)track->timescale / (int64_t)track->timebase_den;
			s_wb32(s, (uint32_t)offset);
		}

		*samples_mdat_offset += smp->size;
	}

	return write_box_size(s, start);
}

/// 8.8.6 Track Fragment Box
static size_t mp4_write_traf(struct mp4_mux *mux, struct mp4_track *track, int64_t moof_start, uint32_t moof_size,
			     uint64_t *samples_mdat_offset)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "traf");

	// tfhd
	mp4_write_tfhd(mux, track, moof_start);

	// tfdt
	mp4_write_tfdt(mux, track);

	// trun
	mp4_write_trun(mux, track, moof_size, samples_mdat_offset);

	return write_box_size(s, start);
}

/// 8.8.4 Movie Fragment Box
static size_t mp4_write_moof(struct mp4_mux *mux, uint32_t moof_size, int64_t moof_start)
{
	struct serializer *s = mux->serializer;
	int64_t start = serializer_get_pos(s);

	write_box(s, 0, "moof");

	mp4_write_mfhd(mux);

	/* Track current mdat offset across tracks */
	uint64_t samples_mdat_offset = 0;

	// traf boxes
	for (size_t i = 0; i < mux->tracks.num; i++) {
		struct mp4_track *track = &mux->tracks.array[i];
		/* Skip tracks that do not have any samples */
		if (!track->fragment_samples.num)
			continue;

		mp4_write_traf(mux, track, moof_start, moof_size, &samples_mdat_offset);
	}

	return write_box_size(s, start);
}

/* ========================================================================== */
/* Chapter packets                                                            */

static void mp4_create_chapter_pkt(struct encoder_packet *pkt, int64_t dts_usec, const char *name)
{
	int64_t dts = dts_usec / 1000; // chapter track uses a ms timebase

	pkt->pts = dts;
	pkt->dts = dts;
	pkt->dts_usec = dts_usec;
	pkt->timebase_num = 1;
	pkt->timebase_den = 1000;

	/* Serialize with data with ref count */
	struct serializer s;
	struct array_output_data ao;
	array_output_serializer_init(&s, &ao);

	size_t len = min(strlen(name), UINT16_MAX);
	long refs = 1;

	/* encoder_packet refs */
	s_write(&s, &refs, sizeof(refs));
	/* actual packet data */
	s_wb16(&s, (uint16_t)len);
	s_write(&s, name, len);
	s_write(&s, &CHAPTER_PKT_FOOTER, sizeof(CHAPTER_PKT_FOOTER));

	pkt->data = (void *)(ao.bytes.array + sizeof(long));
	pkt->size = ao.bytes.num - sizeof(long);
}

/* ========================================================================== */
/* Encoder packet processing and fragment writer                              */

static inline int64_t packet_pts_usec(struct encoder_packet *packet)
{
	return packet->pts * 1000000 / packet->timebase_den;
}

static inline struct encoder_packet *get_pkt_at(struct deque *dq, size_t idx)
{
	return deque_data(dq, idx * sizeof(struct encoder_packet));
}

static inline uint64_t get_longest_track_duration(struct mp4_mux *mux)
{
	uint64_t dur = 0;

	for (size_t i = 0; i < mux->tracks.num; i++) {
		struct mp4_track *track = &mux->tracks.array[i];
		uint64_t track_dur = util_mul_div64(track->duration, 1000, track->timebase_den);

		if (track_dur > dur)
			dur = track_dur;
	}

	return dur;
}

static void process_packets(struct mp4_mux *mux, struct mp4_track *track, uint64_t *mdat_size)
{
	size_t count = track->packets.size / sizeof(struct encoder_packet);

	if (!count)
		return;

	/* Only iterate upt to penultimate packet so we can determine duration
	 * for all processed packets. */
	for (size_t i = 0; i < count - 1; i++) {
		struct encoder_packet *pkt = get_pkt_at(&track->packets, i);

		if (mux->next_frag_pts && packet_pts_usec(pkt) >= mux->next_frag_pts)
			break;

		struct encoder_packet *next = get_pkt_at(&track->packets, i + 1);

		/* Duration is just distance between current and next DTS. */
		uint32_t duration = (uint32_t)(next->dts - pkt->dts);
		uint32_t sample_count = 1;
		uint32_t size = (uint32_t)pkt->size;
		int32_t offset = (int32_t)(pkt->pts - pkt->dts);

		/* When using negative CTS, subtract DTS-PTS offset. */
		if (track->type == TRACK_VIDEO && mux->flags & MP4_USE_NEGATIVE_CTS) {
			if (!track->offsets.num)
				track->dts_offset = offset;

			offset -= track->dts_offset;
		}

		/* Create temporary sample information for moof */
		struct fragment_sample *smp = da_push_back_new(track->fragment_samples);
		smp->size = size;
		smp->offset = offset;
		smp->duration = duration;

		*mdat_size += size;

		/* Update global sample information for full moov */
		track->duration += duration;

		if (track->sample_size) {
			/* Adjust duration/count for fixed sample size */
			sample_count = size / track->sample_size;
			duration = 1;
		}

		if (!track->samples)
			track->first_pts = pkt->pts;

		track->samples += sample_count;

		/* If delta (duration) matche sprevious, increment counter,
		 * otherwise create a new entry. */
		if (track->deltas.num == 0 || track->deltas.array[track->deltas.num - 1].delta != duration) {
			struct sample_delta *new = da_push_back_new(track->deltas);
			new->delta = duration;
			new->count = sample_count;
		} else {
			track->deltas.array[track->deltas.num - 1].count += sample_count;
		}

		if (!track->sample_size)
			da_push_back(track->sample_sizes, &size);

		if (track->type != TRACK_VIDEO)
			continue;

		if (pkt->keyframe)
			da_push_back(track->sync_samples, &track->samples);

		/* Only require ctts box if offet is non-zero */
		if (offset && !track->needs_ctts)
			track->needs_ctts = true;

		/* If dts-pts offset matche sprevious, increment counter,
		 * otherwise create a new entry. */
		if (track->offsets.num == 0 || track->offsets.array[track->offsets.num - 1].offset != offset) {
			struct sample_offset *new = da_push_back_new(track->offsets);
			new->offset = offset;
			new->count = 1;
		} else {
			track->offsets.array[track->offsets.num - 1].count += 1;
		}
	}
}

/* Write track data to file */
static void write_packets(struct mp4_mux *mux, struct mp4_track *track)
{
	struct serializer *s = mux->serializer;

	size_t count = track->packets.size / sizeof(struct encoder_packet);
	if (!count || !track->fragment_samples.num)
		return;

	struct chunk *chk = da_push_back_new(track->chunks);
	chk->offset = serializer_get_pos(s);
	chk->samples = (uint32_t)track->fragment_samples.num;

	for (size_t i = 0; i < track->fragment_samples.num; i++) {
		struct encoder_packet pkt;
		deque_pop_front(&track->packets, &pkt, sizeof(struct encoder_packet));
		s_write(s, pkt.data, pkt.size);
		obs_encoder_packet_release(&pkt);
	}

	chk->size = (uint32_t)(serializer_get_pos(s) - chk->offset);

	/* Fixup sample count for fixed-size codecs */
	if (track->sample_size)
		chk->samples = chk->size / track->sample_size;

	da_clear(track->fragment_samples);
}

static void mp4_flush_fragment(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	// Write file header if not already done
	if (!mux->fragments_written) {
		mp4_write_ftyp(mux, true);
		/* Placeholder to write mdat header during soft-remux */
		mux->placeholder_offset = serializer_get_pos(s);
		mp4_write_free(mux);
	}

	// Array output as temporary buffer to avoid sending seeks to disk
	struct serializer as;
	struct array_output_data aod;
	array_output_serializer_init(&as, &aod);
	mux->serializer = &as;

	// Write initial incomplete moov (because fragmentation)
	if (!mux->fragments_written) {
		mp4_write_moov(mux, true);
		s_write(s, aod.bytes.array, aod.bytes.num);
		array_output_serializer_reset(&aod);
	}

	mux->fragments_written++;

	/* --------------------------------------------------------- */
	/* Analyse packets and create fragment moof.                 */

	uint64_t mdat_size = 8;

	for (size_t idx = 0; idx < mux->tracks.num; idx++) {
		struct mp4_track *track = &mux->tracks.array[idx];
		process_packets(mux, track, &mdat_size);
	}

	if (!mux->next_frag_pts && mux->chapter_track) {
		// Create dummy chapter marker at the end so duration is correct
		uint64_t duration = get_longest_track_duration(mux);
		struct encoder_packet pkt;
		mp4_create_chapter_pkt(&pkt, (int64_t)duration * 1000, "Dummy");
		deque_push_back(&mux->chapter_track->packets, &pkt, sizeof(struct encoder_packet));

		process_packets(mux, mux->chapter_track, &mdat_size);
	}

	// write moof once to get size
	int64_t moof_start = serializer_get_pos(s);
	size_t moof_size = mp4_write_moof(mux, 0, moof_start);
	array_output_serializer_reset(&aod);

	// write moof again with known size
	mp4_write_moof(mux, (uint32_t)moof_size, moof_start);

	// Write to output and restore real serializer
	s_write(s, aod.bytes.array, aod.bytes.num);
	mux->serializer = s;
	array_output_serializer_free(&aod);

	/* --------------------------------------------------------- */
	/* Write audio and video samples (in chunks). Also update    */
	/* global chunk and sample information for final moov.       */

	if (mdat_size > UINT32_MAX) {
		s_wb32(s, 1);
		s_write(s, "mdat", 4);
		s_wb64(s, mdat_size + 8);
	} else {
		s_wb32(s, (uint32_t)mdat_size);
		s_write(s, "mdat", 4);
	}

	for (size_t i = 0; i < mux->tracks.num; i++) {
		struct mp4_track *track = &mux->tracks.array[i];
		write_packets(mux, track);
	}

	/* Only write chapter packets on final flush. */
	if (!mux->next_frag_pts && mux->chapter_track)
		write_packets(mux, mux->chapter_track);

	mux->next_frag_pts = 0;
}

/* ========================================================================== */
/* Track object functions                                                     */

static inline void track_insert_packet(struct mp4_track *track, struct encoder_packet *pkt)
{
	int64_t pts_usec = packet_pts_usec(pkt);
	if (pts_usec > track->last_pts_usec)
		track->last_pts_usec = pts_usec;

	deque_push_back(&track->packets, pkt, sizeof(struct encoder_packet));
}

static inline uint32_t get_sample_size(struct mp4_track *track)
{
	audio_t *audio = obs_encoder_audio(track->encoder);
	if (!audio)
		return 0;

	const struct audio_output_info *info = audio_output_get_info(audio);
	uint32_t channels = get_audio_channels(info->speakers);

	switch (track->codec) {
	case CODEC_PCM_F32:
		return channels * 4; // 4 bytes per sample (32-bit)
	case CODEC_PCM_I24:
		return channels * 3; // 3 bytes per sample (24-bit)
	case CODEC_PCM_I16:
		return channels * 2; // 2 bytes per sample (16-bit)
	default:
		return 0;
	}
}

static inline enum mp4_codec get_codec(obs_encoder_t *enc)
{
	const char *codec = obs_encoder_get_codec(enc);

	if (strcmp(codec, "h264") == 0)
		return CODEC_H264;
	if (strcmp(codec, "hevc") == 0)
		return CODEC_HEVC;
	if (strcmp(codec, "av1") == 0)
		return CODEC_AV1;
	if (strcmp(codec, "prores") == 0)
		return CODEC_PRORES;
	if (strcmp(codec, "aac") == 0)
		return CODEC_AAC;
	if (strcmp(codec, "opus") == 0)
		return CODEC_OPUS;
	if (strcmp(codec, "flac") == 0)
		return CODEC_FLAC;
	if (strcmp(codec, "alac") == 0)
		return CODEC_ALAC;
	if (strcmp(codec, "pcm_s16le") == 0)
		return CODEC_PCM_I16;
	if (strcmp(codec, "pcm_s24le") == 0)
		return CODEC_PCM_I24;
	if (strcmp(codec, "pcm_f32le") == 0)
		return CODEC_PCM_F32;

	return CODEC_UNKNOWN;
}

static inline void add_track(struct mp4_mux *mux, obs_encoder_t *enc)
{
	struct mp4_track *track = da_push_back_new(mux->tracks);

	track->type = obs_encoder_get_type(enc) == OBS_ENCODER_VIDEO ? TRACK_VIDEO : TRACK_AUDIO;
	track->encoder = obs_encoder_get_ref(enc);
	track->codec = get_codec(enc);
	track->track_id = ++mux->track_ctr;

	/* Set timebase/timescale */
	if (track->type == TRACK_VIDEO) {
		video_t *video = obs_encoder_video(enc);
		const struct video_output_info *info = video_output_get_info(video);
		track->timebase_num = info->fps_den;
		track->timebase_den = info->fps_num;

		track->timescale = track->timebase_den;
	} else {
		uint32_t sample_rate = obs_encoder_get_sample_rate(enc);
		/* Opus is always 48 kHz */
		if (track->codec == CODEC_OPUS)
			sample_rate = 48000;
		track->timebase_num = 1;
		track->timebase_den = sample_rate;
		track->timescale = sample_rate;
	}

	/* Set sample size (if fixed) */
	if (track->type == TRACK_AUDIO)
		track->sample_size = get_sample_size(track);
}

static inline void add_chapter_track(struct mp4_mux *mux)
{
	mux->chapter_track = bzalloc(sizeof(struct mp4_track));
	mux->chapter_track->type = TRACK_CHAPTERS;
	mux->chapter_track->codec = CODEC_TEXT;
	mux->chapter_track->timescale = 1000;
	mux->chapter_track->timebase_num = 1;
	mux->chapter_track->timebase_den = 1000;
	mux->chapter_track->track_id = ++mux->track_ctr;
}

static inline void free_packets(struct deque *dq)
{
	size_t num = dq->size / sizeof(struct encoder_packet);

	for (size_t i = 0; i < num; i++) {
		struct encoder_packet pkt;
		deque_pop_front(dq, &pkt, sizeof(struct encoder_packet));
		obs_encoder_packet_release(&pkt);
	}
}

static inline void free_track(struct mp4_track *track)
{
	if (!track)
		return;

	obs_encoder_release(track->encoder);

	free_packets(&track->packets);
	deque_free(&track->packets);

	da_free(track->sample_sizes);
	da_free(track->chunks);
	da_free(track->deltas);
	da_free(track->offsets);
	da_free(track->sync_samples);
	da_free(track->fragment_samples);
}

/* ===========================================================================*/
/* API */

struct mp4_mux *mp4_mux_create(obs_output_t *output, struct serializer *serializer, enum mp4_mux_flags flags,
			       enum mp4_flavor flavor)
{
	struct mp4_mux *mux = bzalloc(sizeof(struct mp4_mux));

	mux->output = output;
	mux->serializer = serializer;
	mux->flags = flags;
	mux->flavor = flavor;
	/* Timestamp is based on 1904 rather than 1970. */
	mux->creation_time = time(NULL) + 0x7C25B080;

	if (flavor == FLAVOR_MOV && mux->creation_time > UINT32_MAX) {
		/* This will only happen in 2040 but better safe than sorry! */
		warn("Creation time too large for MOV, setting to 0 (unset).");
		mux->creation_time = 0;
	}

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *enc = obs_output_get_video_encoder2(output, i);
		if (!enc)
			continue;
		add_track(mux, enc);
	}

	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		obs_encoder_t *enc = obs_output_get_audio_encoder(output, i);
		if (!enc)
			continue;
		add_track(mux, enc);
	}

	return mux;
}

void mp4_mux_destroy(struct mp4_mux *mux)
{
	for (size_t i = 0; i < mux->tracks.num; i++)
		free_track(&mux->tracks.array[i]);

	free_track(mux->chapter_track);
	bfree(mux->chapter_track);
	da_free(mux->tracks);
	bfree(mux);
}

bool mp4_mux_submit_packet(struct mp4_mux *mux, struct encoder_packet *pkt)
{
	struct mp4_track *track = NULL;
	struct encoder_packet parsed_packet;
	enum obs_encoder_type type = pkt->type;
	bool fragment_ready = mux->next_frag_pts > 0;

	for (size_t i = 0; i < mux->tracks.num; i++) {
		struct mp4_track *tmp = &mux->tracks.array[i];

		fragment_ready = fragment_ready && tmp->last_pts_usec >= mux->next_frag_pts;

		if (tmp->encoder == pkt->encoder)
			track = tmp;
	}

	if (!track) {
		warn("Could not find track for packet of type %s with "
		     "track id %zu!",
		     type == OBS_ENCODER_VIDEO ? "video" : "audio", pkt->track_idx);
		return false;
	}

	/* If all tracks have caught up to the keyframe we want to fragment on,
	 * flush the current fragment to disk. */
	if (fragment_ready)
		mp4_flush_fragment(mux);

	if (type == OBS_ENCODER_AUDIO) {
		obs_encoder_packet_ref(&parsed_packet, pkt);
	} else {
		if (track->codec == CODEC_H264)
			obs_parse_avc_packet(&parsed_packet, pkt);
		else if (track->codec == CODEC_HEVC)
			obs_parse_hevc_packet(&parsed_packet, pkt);
		else if (track->codec == CODEC_AV1)
			obs_parse_av1_packet(&parsed_packet, pkt);
		else if (track->codec == CODEC_PRORES)
			obs_encoder_packet_ref(&parsed_packet, pkt);

		/* Set fragmentation PTS if packet is keyframe and PTS > 0 */
		if (parsed_packet.keyframe && parsed_packet.pts > 0) {
			mux->next_frag_pts = packet_pts_usec(&parsed_packet);
		}
	}

	track_insert_packet(track, &parsed_packet);

	return true;
}

bool mp4_mux_add_chapter(struct mp4_mux *mux, int64_t dts_usec, const char *name)
{
	if (dts_usec < 0)
		return false;
	if (!mux->chapter_track)
		add_chapter_track(mux);

	/* To work correctly there needs to be a chapter at PTS 0,
	 * create that here if necessary. */
	if (dts_usec > 0 && mux->chapter_track->packets.size == 0) {
		mp4_mux_add_chapter(mux, 0, obs_module_text("MP4Output.StartChapter"));
	}

	/* Create packets that will be muxed on final flush */
	struct encoder_packet pkt;
	mp4_create_chapter_pkt(&pkt, dts_usec, name);
	track_insert_packet(mux->chapter_track, &pkt);

	return true;
}

bool mp4_mux_finalise(struct mp4_mux *mux)
{
	struct serializer *s = mux->serializer;

	/* Flush remaining audio/video samples as final fragment. */
	info("Flushing final fragment...");

	/* Set target PTS to zero to indicate that we want to flush all
	 * the remaining packets */
	mux->next_frag_pts = 0;
	mp4_flush_fragment(mux);

	info("Number of fragments: %u", mux->fragments_written);

	if (mux->flags & MP4_SKIP_FINALISATION) {
		warn("Skipping finalization!");
		return true;
	}

	int64_t data_end = serializer_get_pos(s);

	/* ---------------------------------------- */
	/* Write full moov box                      */

	/* Use array serializer for moov data as this will do a lot
	 * of seeks to write size values of variable-size boxes. */
	struct serializer fs;
	struct array_output_data ao;
	array_output_serializer_init(&fs, &ao);

	mux->serializer = &fs;

	mp4_write_moov(mux, false);
	s_write(s, ao.bytes.array, ao.bytes.num);
	info("Full moov size: %zu KiB", ao.bytes.num / 1024);

	mux->serializer = s; // restore real serializer
	array_output_serializer_free(&ao);

	/* ---------------------------------------- */
	/* Overwrite file header (ftyp + free/moov) */

	serializer_seek(s, 0, SERIALIZE_SEEK_START);
	mp4_write_ftyp(mux, false);

	size_t data_size = data_end - mux->placeholder_offset;
	serializer_seek(s, (int64_t)mux->placeholder_offset, SERIALIZE_SEEK_START);

	/* If data is more than 4 GiB the mdat header becomes 16 bytes, hence
	 * why we create a 16-byte placeholder "free" box at the start. */
	if (data_size > UINT32_MAX) {
		s_wb32(s, 1); // 1 = use "largesize" field instead
		s_write(s, "mdat", 4);
		s_wb64(s, data_size); // largesize (64-bit)
	} else {
		s_wb32(s, (uint32_t)data_size);
		s_write(s, "mdat", 4);
	}

	info("Final mdat size: %zu KiB", data_size / 1024);
	return true;
}
