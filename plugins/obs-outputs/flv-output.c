/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <stdio.h>
#include <obs-module.h>
#include <obs-avc.h>
#ifdef ENABLE_HEVC
#include "rtmp-hevc.h"
#include <obs-hevc.h>
#endif
#include "rtmp-av1.h"
#include <util/platform.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>
#include "flv-mux.h"

#define do_log(level, format, ...)                \
	blog(level, "[flv output: '%s'] " format, \
	     obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

struct flv_output {
	obs_output_t *output;
	struct dstr path;
	FILE *file;
	volatile bool active;
	volatile bool stopping;
	uint64_t stop_ts;
	bool sent_headers;
	int64_t last_packet_ts;

	enum audio_id_t audio_codec[MAX_OUTPUT_AUDIO_ENCODERS];
	enum video_id_t video_codec[MAX_OUTPUT_VIDEO_ENCODERS];

	pthread_mutex_t mutex;

	bool got_first_video;
	int32_t start_dts_offset;
};

/* Adapted from FFmpeg's libavutil/pixfmt.h
 *
 * Renamed to make it apparent that these are not imported as this module does
 * not use or link against FFmpeg.
 */

/* clang-format off */

/**
 * Chromaticity Coordinates of the Source Primaries
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.1 and ITU-T H.273.
 */
enum OBSColorPrimaries {
	OBSCOL_PRI_RESERVED0    =  0,
	OBSCOL_PRI_BT709        =  1, ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP 177 Annex B
	OBSCOL_PRI_UNSPECIFIED  =  2,
	OBSCOL_PRI_RESERVED     =  3,
	OBSCOL_PRI_BT470M       =  4, ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	OBSCOL_PRI_BT470BG      =  5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
	OBSCOL_PRI_SMPTE170M    =  6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
	OBSCOL_PRI_SMPTE240M    =  7, ///< identical to above, also called "SMPTE C" even though it uses D65
	OBSCOL_PRI_FILM         =  8, ///< colour filters using Illuminant C
	OBSCOL_PRI_BT2020       =  9, ///< ITU-R BT2020
	OBSCOL_PRI_SMPTE428     = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
	OBSCOL_PRI_SMPTEST428_1 = OBSCOL_PRI_SMPTE428,
	OBSCOL_PRI_SMPTE431     = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
	OBSCOL_PRI_SMPTE432     = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
	OBSCOL_PRI_EBU3213      = 22, ///< EBU Tech. 3213-E (nothing there) / one of JEDEC P22 group phosphors
	OBSCOL_PRI_JEDEC_P22    = OBSCOL_PRI_EBU3213,
	OBSCOL_PRI_NB                 ///< Not part of ABI
};

/**
 * Color Transfer Characteristic
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.2.
 */
enum OBSColorTransferCharacteristic {
	OBSCOL_TRC_RESERVED0    =  0,
	OBSCOL_TRC_BT709        =  1, ///< also ITU-R BT1361
	OBSCOL_TRC_UNSPECIFIED  =  2,
	OBSCOL_TRC_RESERVED     =  3,
	OBSCOL_TRC_GAMMA22      =  4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
	OBSCOL_TRC_GAMMA28      =  5, ///< also ITU-R BT470BG
	OBSCOL_TRC_SMPTE170M    =  6, ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
	OBSCOL_TRC_SMPTE240M    =  7,
	OBSCOL_TRC_LINEAR       =  8, ///< "Linear transfer characteristics"
	OBSCOL_TRC_LOG          =  9, ///< "Logarithmic transfer characteristic (100:1 range)"
	OBSCOL_TRC_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
	OBSCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
	OBSCOL_TRC_BT1361_ECG   = 12, ///< ITU-R BT1361 Extended Colour Gamut
	OBSCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
	OBSCOL_TRC_BT2020_10    = 14, ///< ITU-R BT2020 for 10-bit system
	OBSCOL_TRC_BT2020_12    = 15, ///< ITU-R BT2020 for 12-bit system
	OBSCOL_TRC_SMPTE2084    = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
	OBSCOL_TRC_SMPTEST2084  = OBSCOL_TRC_SMPTE2084,
	OBSCOL_TRC_SMPTE428     = 17, ///< SMPTE ST 428-1
	OBSCOL_TRC_SMPTEST428_1 = OBSCOL_TRC_SMPTE428,
	OBSCOL_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
	OBSCOL_TRC_NB                 ///< Not part of ABI
};

/**
 * YUV Colorspace Type
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.3.
 */
enum OBSColorSpace {
	OBSCOL_SPC_RGB                =  0, ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB), YZX and ST 428-1
	OBSCOL_SPC_BT709              =  1, ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / derived in SMPTE RP 177 Annex B
	OBSCOL_SPC_UNSPECIFIED        =  2,
	OBSCOL_SPC_RESERVED           =  3, ///< reserved for future use by ITU-T and ISO/IEC just like 15-255 are
	OBSCOL_SPC_FCC                =  4, ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	OBSCOL_SPC_BT470BG            =  5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
	OBSCOL_SPC_SMPTE170M          =  6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC / functionally identical to above
	OBSCOL_SPC_SMPTE240M          =  7, ///< derived from 170M primaries and D65 white point, 170M is derived from BT470 System M's primaries
	OBSCOL_SPC_YCGCO              =  8, ///< used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
	OBSCOL_SPC_YCOCG              =  OBSCOL_SPC_YCGCO,
	OBSCOL_SPC_BT2020_NCL         =  9, ///< ITU-R BT2020 non-constant luminance system
	OBSCOL_SPC_BT2020_CL          = 10, ///< ITU-R BT2020 constant luminance system
	OBSCOL_SPC_SMPTE2085          = 11, ///< SMPTE 2085, Y'D'zD'x
	OBSCOL_SPC_CHROMA_DERIVED_NCL = 12, ///< Chromaticity-derived non-constant luminance system
	OBSCOL_SPC_CHROMA_DERIVED_CL  = 13, ///< Chromaticity-derived constant luminance system
	OBSCOL_SPC_ICTCP              = 14, ///< ITU-R BT.2100-0, ICtCp
	OBSCOL_SPC_NB                       ///< Not part of ABI
};

/* clang-format on */

static inline bool stopping(struct flv_output *stream)
{
	return os_atomic_load_bool(&stream->stopping);
}

static inline bool active(struct flv_output *stream)
{
	return os_atomic_load_bool(&stream->active);
}

static const char *flv_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FLVOutput");
}

static void flv_output_stop(void *data, uint64_t ts);

static void flv_output_destroy(void *data)
{
	struct flv_output *stream = data;

	pthread_mutex_destroy(&stream->mutex);
	dstr_free(&stream->path);
	bfree(stream);
}

static void *flv_output_create(obs_data_t *settings, obs_output_t *output)
{
	struct flv_output *stream = bzalloc(sizeof(struct flv_output));
	stream->output = output;
	pthread_mutex_init(&stream->mutex, NULL);

	UNUSED_PARAMETER(settings);
	return stream;
}

static int write_packet(struct flv_output *stream,
			struct encoder_packet *packet, bool is_header)
{
	uint8_t *data;
	size_t size;
	int ret = 0;

	stream->last_packet_ts = get_ms_time(packet, packet->dts);

	flv_packet_mux(packet, is_header ? 0 : stream->start_dts_offset, &data,
		       &size, is_header);
	fwrite(data, 1, size, stream->file);
	bfree(data);

	return ret;
}

static int write_packet_ex(struct flv_output *stream,
			   struct encoder_packet *packet, bool is_header,
			   bool is_footer, size_t idx)
{
	uint8_t *data;
	size_t size = 0;
	int ret = 0;

	if (is_header) {
		flv_packet_start(packet, stream->video_codec[idx], &data, &size,
				 idx);
	} else if (is_footer) {
		flv_packet_end(packet, stream->video_codec[idx], &data, &size,
			       idx);
	} else {
		flv_packet_frames(packet, stream->video_codec[idx],
				  stream->start_dts_offset, &data, &size, idx);
	}

	fwrite(data, 1, size, stream->file);
	bfree(data);

	// manually created packets
	if (is_header || is_footer)
		bfree(packet->data);
	else
		obs_encoder_packet_release(packet);

	return ret;
}

static int write_audio_packet_ex(struct flv_output *stream,
				 struct encoder_packet *packet, bool is_header,
				 size_t idx)
{
	uint8_t *data;
	size_t size = 0;
	int ret = 0;

	if (is_header) {
		flv_packet_audio_start(packet, stream->audio_codec[idx], &data,
				       &size, idx);
	} else {
		flv_packet_audio_frames(packet, stream->audio_codec[idx],
					stream->start_dts_offset, &data, &size,
					idx);
	}

	fwrite(data, 1, size, stream->file);
	bfree(data);

	return ret;
}

static void write_meta_data(struct flv_output *stream)
{
	uint8_t *meta_data;
	size_t meta_data_size;

	flv_meta_data(stream->output, &meta_data, &meta_data_size, true);
	fwrite(meta_data, 1, meta_data_size, stream->file);
	bfree(meta_data);
}

static bool write_audio_header(struct flv_output *stream, size_t idx)
{
	obs_output_t *context = stream->output;
	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, idx);

	struct encoder_packet packet = {.type = OBS_ENCODER_AUDIO,
					.timebase_den = 1};

	if (!aencoder)
		return false;

	if (obs_encoder_get_extra_data(aencoder, &packet.data, &packet.size)) {
		if (idx == 0) {
			write_packet(stream, &packet, true);
		} else {
			write_audio_packet_ex(stream, &packet, true, idx);
		}
	}

	return true;
}

static bool write_video_header(struct flv_output *stream, size_t idx)
{
	obs_output_t *context = stream->output;
	obs_encoder_t *vencoder = obs_output_get_video_encoder2(context, idx);
	uint8_t *header;
	size_t size;

	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO,
					.timebase_den = 1,
					.keyframe = true};

	if (!vencoder)
		return false;

	if (!obs_encoder_get_extra_data(vencoder, &header, &size))
		return false;

	switch (stream->video_codec[idx]) {
	case CODEC_NONE:
		do_log(LOG_ERROR,
		       "Codec not initialized for track %zu while sending header",
		       idx);
		return false;

	case CODEC_H264:
		packet.size = obs_parse_avc_header(&packet.data, header, size);
		// Always send H.264 on track 0 as old style for compatibility.
		if (idx == 0) {
			write_packet(stream, &packet, true);
		} else {
			write_packet_ex(stream, &packet, true, false, idx);
		}
		return true;
	case CODEC_HEVC:
#ifdef ENABLE_HEVC
		packet.size = obs_parse_hevc_header(&packet.data, header, size);
		break;
#else
		return false;
#endif
	case CODEC_AV1:
		packet.size = obs_parse_av1_header(&packet.data, header, size);
		break;
	}
	write_packet_ex(stream, &packet, true, false, idx);

	return true;
}

// only returns false if there's an error, not if no metadata needs to be sent
static bool write_video_metadata(struct flv_output *stream, size_t idx)
{
	// send metadata only if HDR
	obs_encoder_t *encoder =
		obs_output_get_video_encoder2(stream->output, idx);
	if (!encoder)
		return false;

	video_t *video = obs_encoder_video(encoder);
	if (!video)
		return false;

	const struct video_output_info *info = video_output_get_info(video);
	enum video_colorspace colorspace = info->colorspace;
	if (!(colorspace == VIDEO_CS_2100_PQ ||
	      colorspace == VIDEO_CS_2100_HLG))
		return true;

	// Y2023 spec
	if (stream->video_codec[idx] == CODEC_H264)
		return true;

	uint8_t *data;
	size_t size;

	enum video_format format = info->format;

	int bits_per_raw_sample;
	switch (format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
	case VIDEO_FORMAT_I210:
		bits_per_raw_sample = 10;
		break;
	case VIDEO_FORMAT_I412:
	case VIDEO_FORMAT_YA2L:
		bits_per_raw_sample = 12;
		break;
	default:
		bits_per_raw_sample = 8;
	}

	int pri = 0;
	int trc = 0;
	int spc = 0;
	switch (colorspace) {
	case VIDEO_CS_601:
		pri = OBSCOL_PRI_SMPTE170M;
		trc = OBSCOL_PRI_SMPTE170M;
		spc = OBSCOL_PRI_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		pri = OBSCOL_PRI_BT709;
		trc = OBSCOL_PRI_BT709;
		spc = OBSCOL_PRI_BT709;
		break;
	case VIDEO_CS_SRGB:
		pri = OBSCOL_PRI_BT709;
		trc = OBSCOL_TRC_IEC61966_2_1;
		spc = OBSCOL_PRI_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		pri = OBSCOL_PRI_BT2020;
		trc = OBSCOL_TRC_SMPTE2084;
		spc = OBSCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		pri = OBSCOL_PRI_BT2020;
		trc = OBSCOL_TRC_ARIB_STD_B67;
		spc = OBSCOL_SPC_BT2020_NCL;
	}

	int max_luminance = 0;
	if (trc == OBSCOL_TRC_ARIB_STD_B67)
		max_luminance = 1000;
	else if (trc == OBSCOL_TRC_SMPTE2084)
		max_luminance = (int)obs_get_video_hdr_nominal_peak_level();

	flv_packet_metadata(stream->video_codec[idx], &data, &size,
			    bits_per_raw_sample, pri, trc, spc, 0,
			    max_luminance, idx);

	fwrite(data, 1, size, stream->file);
	bfree(data);

	return true;
}

static void write_headers(struct flv_output *stream)
{
	for (size_t i = 0; i < MAX_OUTPUT_AUDIO_ENCODERS; i++) {
		obs_encoder_t *enc =
			obs_output_get_audio_encoder(stream->output, i);
		if (!enc)
			break;

		const char *codec = obs_encoder_get_codec(enc);
		stream->audio_codec[i] = to_audio_type(codec);
	}

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *enc =
			obs_output_get_video_encoder2(stream->output, i);
		if (!enc)
			break;

		const char *codec = obs_encoder_get_codec(enc);
		stream->video_codec[i] = to_video_type(codec);
	}

	write_meta_data(stream);
	write_audio_header(stream, 0);

	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *enc =
			obs_output_get_video_encoder2(stream->output, i);
		if (!enc)
			continue;

		if (!write_video_header(stream, i) ||
		    !write_video_metadata(stream, i))
			return;
	}

	for (size_t i = 1; write_audio_header(stream, i); i++)
		;
}

static bool write_video_footer(struct flv_output *stream, size_t idx)
{
	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO,
					.timebase_den = 1,
					.keyframe = false};
	packet.size = 0;

	return write_packet_ex(stream, &packet, false, true, idx) >= 0;
}

static inline bool write_footers(struct flv_output *stream)
{
	for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
		obs_encoder_t *encoder =
			obs_output_get_video_encoder2(stream->output, i);
		if (!encoder)
			continue;

		if (i == 0 && stream->video_codec[i] == CODEC_H264)
			continue;

		if (!write_video_footer(stream, i))
			return false;
	}

	return true;
}

static bool flv_output_start(void *data)
{
	struct flv_output *stream = data;
	obs_data_t *settings;
	const char *path;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	stream->got_first_video = false;
	stream->sent_headers = false;
	os_atomic_set_bool(&stream->stopping, false);

	/* get path */
	settings = obs_output_get_settings(stream->output);
	path = obs_data_get_string(settings, "path");
	dstr_copy(&stream->path, path);
	obs_data_release(settings);

	stream->file = os_fopen(stream->path.array, "wb");
	if (!stream->file) {
		warn("Unable to open FLV file '%s'", stream->path.array);
		return false;
	}

	/* write headers and start capture */
	os_atomic_set_bool(&stream->active, true);
	obs_output_begin_data_capture(stream->output, 0);

	info("Writing FLV file '%s'...", stream->path.array);
	return true;
}

static void flv_output_stop(void *data, uint64_t ts)
{
	struct flv_output *stream = data;
	stream->stop_ts = ts / 1000;
	os_atomic_set_bool(&stream->stopping, true);
}

static void flv_output_actual_stop(struct flv_output *stream, int code)
{
	os_atomic_set_bool(&stream->active, false);

	if (stream->file) {
		write_footers(stream);
		write_file_info(stream->file, stream->last_packet_ts,
				os_ftelli64(stream->file));

		fclose(stream->file);
	}
	if (code) {
		obs_output_signal_stop(stream->output, code);
	} else {
		obs_output_end_data_capture(stream->output);
	}

	info("FLV file output complete");
}

static void flv_output_data(void *data, struct encoder_packet *packet)
{
	struct flv_output *stream = data;
	struct encoder_packet parsed_packet;

	pthread_mutex_lock(&stream->mutex);

	if (!active(stream))
		goto unlock;

	if (!packet) {
		flv_output_actual_stop(stream, OBS_OUTPUT_ENCODE_ERROR);
		goto unlock;
	}

	if (stopping(stream)) {
		if (packet->sys_dts_usec >= (int64_t)stream->stop_ts) {
			flv_output_actual_stop(stream, 0);
			goto unlock;
		}
	}

	if (!stream->sent_headers) {
		write_headers(stream);
		stream->sent_headers = true;
	}

	if (packet->type == OBS_ENCODER_VIDEO) {
		if (!stream->got_first_video) {
			stream->start_dts_offset =
				get_ms_time(packet, packet->dts);
			stream->got_first_video = true;
		}

		switch (stream->video_codec[packet->track_idx]) {
		case CODEC_NONE:
			do_log(LOG_ERROR, "Codec not initialized for track %zu",
			       packet->track_idx);
			goto unlock;

		case CODEC_H264:
			obs_parse_avc_packet(&parsed_packet, packet);
			break;
		case CODEC_HEVC:
#ifdef ENABLE_HEVC
			obs_parse_hevc_packet(&parsed_packet, packet);
			break;
#else
			goto unlock;
#endif
		case CODEC_AV1:
			obs_parse_av1_packet(&parsed_packet, packet);
			break;
		}

		if (stream->video_codec[packet->track_idx] != CODEC_H264 ||
		    (stream->video_codec[packet->track_idx] == CODEC_H264 &&
		     packet->track_idx != 0)) {
			write_packet_ex(stream, &parsed_packet, false, false,
					packet->track_idx);
		} else {
			write_packet(stream, &parsed_packet, false);
		}
		obs_encoder_packet_release(&parsed_packet);
	} else {
		if (packet->track_idx != 0) {
			write_audio_packet_ex(stream, packet, false,
					      packet->track_idx);
		} else {
			write_packet(stream, packet, false);
		}
	}

unlock:
	pthread_mutex_unlock(&stream->mutex);
}

static obs_properties_t *flv_output_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "path",
				obs_module_text("FLVOutput.FilePath"),
				OBS_TEXT_DEFAULT);
	return props;
}

struct obs_output_info flv_output_info = {
	.id = "flv_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK_AV,
#ifdef ENABLE_HEVC
	.encoded_video_codecs = "h264;hevc;av1",
#else
	.encoded_video_codecs = "h264;av1",
#endif
	.encoded_audio_codecs = "aac",
	.get_name = flv_output_getname,
	.create = flv_output_create,
	.destroy = flv_output_destroy,
	.start = flv_output_start,
	.stop = flv_output_stop,
	.encoded_packet = flv_output_data,
	.get_properties = flv_output_properties,
};
