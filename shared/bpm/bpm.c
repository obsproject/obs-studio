#include "obs.h"
#include "bpm-internal.h"

static void render_metrics_time(struct metrics_time *m_time)
{
	/* Generate the RFC3339 time string from the timespec struct, for example:
	 *
	 *   "2024-05-31T12:26:03.591Z"
	 */
	memset(&m_time->rfc3339_str, 0, sizeof(m_time->rfc3339_str));
	strftime(m_time->rfc3339_str, sizeof(m_time->rfc3339_str),
		 "%Y-%m-%dT%T", gmtime(&m_time->tspec.tv_sec));
	sprintf(m_time->rfc3339_str + strlen(m_time->rfc3339_str), ".%03ldZ",
		m_time->tspec.tv_nsec / 1000000);
	m_time->valid = true;
}

static bool update_metrics(obs_output_t *output,
			   const struct encoder_packet *pkt,
			   const struct encoder_packet_time *ept,
			   struct metrics_data *m_track)
{
	if (!output || !pkt || !ept || !m_track) {
		blog(LOG_DEBUG, "%s: Null arguments for track %lu",
		     __FUNCTION__, pkt->track_idx);
		return false;
	}

	// Perform reads on all the counters as close together as possible
	m_track->session_frames_output.curr =
		obs_output_get_total_frames(output);
	m_track->session_frames_dropped.curr =
		obs_output_get_frames_dropped(output);
	m_track->session_frames_rendered.curr = obs_get_total_frames();
	m_track->session_frames_lagged.curr = obs_get_lagged_frames();

	const video_t *video = obs_encoder_video(pkt->encoder);
	if (video) {
		/* video_output_get_total_frames() returns the number of frames
		 * before the framerate decimator. For example, if the OBS session
		 * is rendering at 60fps, and the rendition is set for 30 fps,
		 * the counter will increment by 60 per second, not 30 per second.
		 * For metrics we will consider this value to be the number of
		 * frames input to the obs_encoder_t instance.
		 */
		m_track->rendition_frames_input.curr =
			video_output_get_total_frames(video);
		m_track->rendition_frames_skipped.curr =
			video_output_get_skipped_frames(video);
		/* obs_encoder_get_encoded_frames() returns the number of frames
		 * successfully encoded by the obs_encoder_t instance.
		 */
		m_track->rendition_frames_output.curr =
			obs_encoder_get_encoded_frames(pkt->encoder);
	} else {
		m_track->rendition_frames_input.curr = 0;
		m_track->rendition_frames_skipped.curr = 0;
		m_track->rendition_frames_output.curr = 0;
		blog(LOG_ERROR, "update_metrics(): *video_t==null");
	}

	// Set the diff values to 0 if PTS is 0
	if (pkt->pts == 0) {
		m_track->session_frames_output.diff = 0;
		m_track->session_frames_dropped.diff = 0;
		m_track->session_frames_rendered.diff = 0;
		m_track->session_frames_lagged.diff = 0;
		m_track->rendition_frames_input.diff = 0;
		m_track->rendition_frames_skipped.diff = 0;
		m_track->rendition_frames_output.diff = 0;
		blog(LOG_DEBUG, "update_metrics(): Setting diffs to 0");
	} else {
		// Calculate diff's
		m_track->session_frames_output.diff =
			m_track->session_frames_output.curr -
			m_track->session_frames_output.ref;
		m_track->session_frames_dropped.diff =
			m_track->session_frames_dropped.curr -
			m_track->session_frames_dropped.ref;
		m_track->session_frames_rendered.diff =
			m_track->session_frames_rendered.curr -
			m_track->session_frames_rendered.ref;
		m_track->session_frames_lagged.diff =
			m_track->session_frames_lagged.curr -
			m_track->session_frames_lagged.ref;
		m_track->rendition_frames_input.diff =
			m_track->rendition_frames_input.curr -
			m_track->rendition_frames_input.ref;
		m_track->rendition_frames_skipped.diff =
			m_track->rendition_frames_skipped.curr -
			m_track->rendition_frames_skipped.ref;
		m_track->rendition_frames_output.diff =
			m_track->rendition_frames_output.curr -
			m_track->rendition_frames_output.ref;
	}

	// Update the reference values
	m_track->session_frames_output.ref =
		m_track->session_frames_output.curr;
	m_track->session_frames_dropped.ref =
		m_track->session_frames_dropped.curr;
	m_track->session_frames_rendered.ref =
		m_track->session_frames_rendered.curr;
	m_track->session_frames_lagged.ref =
		m_track->session_frames_lagged.curr;
	m_track->rendition_frames_input.ref =
		m_track->rendition_frames_input.curr;
	m_track->rendition_frames_skipped.ref =
		m_track->rendition_frames_skipped.curr;
	m_track->rendition_frames_output.ref =
		m_track->rendition_frames_output.curr;

	/* BPM Timestamp Message */
	m_track->cts.valid = false;
	m_track->ferts.valid = false;
	m_track->fercts.valid = false;

	/* Generate the timestamp representations for CTS, FER, and FERC.
	 * Check if each is non-zero and that temporal consistency is correct:
	 *   FEC > FERC > CTS
	 * FEC and FERC depends on CTS, and FERC depends on FER, so ensure
	 * we only signal an integral set of timestamps.
	 */
	os_nstime_to_timespec(ept->cts, &m_track->cts.tspec);
	render_metrics_time(&m_track->cts);
	if (ept->fer && (ept->fer > ept->cts)) {
		os_nstime_to_timespec(ept->fer, &m_track->ferts.tspec);
		render_metrics_time(&m_track->ferts);
		if (ept->ferc && (ept->ferc > ept->fer)) {
			os_nstime_to_timespec(ept->ferc,
					      &m_track->fercts.tspec);
			render_metrics_time(&m_track->fercts);
		}
	}

	// Always generate the timestamp representation for PIR
	m_track->pirts.valid = false;
	os_nstime_to_timespec(ept->pir, &m_track->pirts.tspec);
	render_metrics_time(&m_track->pirts);

	/* Log the BPM timestamp and frame counter information. This
	 * provides visibility into the metrics when OBS is started
	 * with "--verbose" and "--unfiltered_log".
	 */
	blog(LOG_DEBUG,
	     "BPM: %s, trk %lu: [CTS|FER-CTS|FERC-FER|PIR-CTS]:[%" PRIu64
	     " ms|%" PRIu64 " ms|%" PRIu64 " us|%" PRIu64
	     " ms], [dts|pts]:[%" PRId64 "|%" PRId64
	     "], S[R:O:D:L],R[I:S:O]:%d:%d:%d:%d:%d:%d:%d",
	     obs_encoder_get_name(pkt->encoder), pkt->track_idx,
	     ept->cts / 1000000, (ept->fer - ept->cts) / 1000000,
	     (ept->ferc - ept->fer) / 1000, (ept->pir - ept->cts) / 1000000,
	     pkt->dts, pkt->pts, m_track->session_frames_rendered.diff,
	     m_track->session_frames_output.diff,
	     m_track->session_frames_dropped.diff,
	     m_track->session_frames_lagged.diff,
	     m_track->rendition_frames_input.diff,
	     m_track->rendition_frames_skipped.diff,
	     m_track->rendition_frames_output.diff);

	return true;
}
void bpm_ts_sei_render(struct metrics_data *m_track)
{
	uint8_t num_timestamps = 0;
	struct serializer s;

	m_track->sei_rendered[BPM_TS_SEI] = false;

	// Initialize the output array here; caller is responsible to free it
	array_output_serializer_init(&s, &m_track->sei_payload[BPM_TS_SEI]);

	// Write the UUID for this SEI message
	s_write(&s, bpm_ts_uuid, sizeof(bpm_ts_uuid));

	// Determine how many timestamps are valid
	if (m_track->cts.valid)
		num_timestamps++;
	if (m_track->ferts.valid)
		num_timestamps++;
	if (m_track->fercts.valid)
		num_timestamps++;
	if (m_track->pirts.valid)
		num_timestamps++;

	/* Encode number of timestamps for this SEI. Upper 4 bits are
	 * set to b0000 (reserved); lower 4-bits num_timestamps - 1.
	 */
	s_w8(&s, (num_timestamps - 1) & 0x0F);

	if (m_track->cts.valid) {
		// Timestamp type
		s_w8(&s, BPM_TS_RFC3339);
		// Write the timestamp event tag (Composition Time Event)
		s_w8(&s, BPM_TS_EVENT_CTS);
		// Write the RFC3339-formatted string, including the null terminator
		s_write(&s, m_track->cts.rfc3339_str,
			strlen(m_track->cts.rfc3339_str) + 1);
	}

	if (m_track->ferts.valid) {
		// Timestamp type
		s_w8(&s, BPM_TS_RFC3339);
		// Write the timestamp event tag (Frame Encode Request Event)
		s_w8(&s, BPM_TS_EVENT_FER);
		// Write the RFC3339-formatted string, including the null terminator
		s_write(&s, m_track->ferts.rfc3339_str,
			strlen(m_track->ferts.rfc3339_str) + 1);
	}

	if (m_track->fercts.valid) {
		// Timestamp type
		s_w8(&s, BPM_TS_RFC3339);
		// Write the timestamp event tag (Frame Encode Request Complete Event)
		s_w8(&s, BPM_TS_EVENT_FERC);
		// Write the RFC3339-formatted string, including the null terminator
		s_write(&s, m_track->fercts.rfc3339_str,
			strlen(m_track->fercts.rfc3339_str) + 1);
	}

	if (m_track->pirts.valid) {
		// Timestamp type
		s_w8(&s, BPM_TS_RFC3339);
		// Write the timestamp event tag (Packet Interleave Request Event)
		s_w8(&s, BPM_TS_EVENT_PIR);
		// Write the RFC3339-formatted string, including the null terminator
		s_write(&s, m_track->pirts.rfc3339_str,
			strlen(m_track->pirts.rfc3339_str) + 1);
	}
	m_track->sei_rendered[BPM_TS_SEI] = true;
}

void bpm_sm_sei_render(struct metrics_data *m_track)
{
	uint8_t num_timestamps = 0;
	uint8_t num_counters = 0;
	struct serializer s;

	m_track->sei_rendered[BPM_SM_SEI] = false;

	// Initialize the output array here; caller is responsible to free it
	array_output_serializer_init(&s, &m_track->sei_payload[BPM_SM_SEI]);

	// Write the UUID for this SEI message
	s_write(&s, bpm_sm_uuid, sizeof(bpm_sm_uuid));

	// Encode number of timestamps for this SEI
	num_timestamps = 1;
	// Upper 4 bits are set to b0000 (reserved); lower 4-bits num_timestamps - 1
	s_w8(&s, (num_timestamps - 1) & 0x0F);
	// Timestamp type
	s_w8(&s, BPM_TS_RFC3339);

	/* Write the timestamp event tag (Packet Interleave Request Event).
	 * Use the PIR_TS timestamp because the data was all collected at that time.
	 */
	s_w8(&s, BPM_TS_EVENT_PIR);
	// Write the RFC3339-formatted string, including the null terminator
	s_write(&s, m_track->pirts.rfc3339_str,
		strlen(m_track->pirts.rfc3339_str) + 1);

	// Session metrics has 4 counters
	num_counters = 4;
	/* Send all the counters with a tag(8-bit):value(32-bit) configuration.
	 * Upper 4 bits are set to b0000 (reserved); lower 4-bits num_counters - 1.
	 */
	s_w8(&s, (num_counters - 1) & 0x0F);
	s_w8(&s, BPM_SM_FRAMES_RENDERED);
	s_wb32(&s, m_track->session_frames_rendered.diff);
	s_w8(&s, BPM_SM_FRAMES_LAGGED);
	s_wb32(&s, m_track->session_frames_lagged.diff);
	s_w8(&s, BPM_SM_FRAMES_DROPPED);
	s_wb32(&s, m_track->session_frames_dropped.diff);
	s_w8(&s, BPM_SM_FRAMES_OUTPUT);
	s_wb32(&s, m_track->session_frames_output.diff);

	m_track->sei_rendered[BPM_SM_SEI] = true;
}

void bpm_erm_sei_render(struct metrics_data *m_track)
{
	uint8_t num_timestamps = 0;
	uint8_t num_counters = 0;
	struct serializer s;

	m_track->sei_rendered[BPM_ERM_SEI] = false;

	// Initialize the output array here; caller is responsible to free it
	array_output_serializer_init(&s, &m_track->sei_payload[BPM_ERM_SEI]);

	// Write the UUID for this SEI message
	s_write(&s, bpm_erm_uuid, sizeof(bpm_erm_uuid));

	// Encode number of timestamps for this SEI
	num_timestamps = 1;
	// Upper 4 bits are set to b0000 (reserved); lower 4-bits num_timestamps - 1
	s_w8(&s, (num_timestamps - 1) & 0x0F);
	// Timestamp type
	s_w8(&s, BPM_TS_RFC3339);

	/* Write the timestamp event tag (Packet Interleave Request Event).
	 * Use the PIRTS timestamp because the data was all collected at that time.
	 */
	s_w8(&s, BPM_TS_EVENT_PIR);
	// Write the RFC3339-formatted string, including the null terminator
	s_write(&s, m_track->pirts.rfc3339_str,
		strlen(m_track->pirts.rfc3339_str) + 1);

	// Encoder rendition metrics has 3 counters
	num_counters = 3;
	/* Send all the counters with a tag(8-bit):value(32-bit) configuration.
	 * Upper 4 bits are set to b0000 (reserved); lower 4-bits num_counters - 1.
	 */
	s_w8(&s, (num_counters - 1) & 0x0F);
	s_w8(&s, BPM_ERM_FRAMES_INPUT);
	s_wb32(&s, m_track->rendition_frames_input.diff);
	s_w8(&s, BPM_ERM_FRAMES_SKIPPED);
	s_wb32(&s, m_track->rendition_frames_skipped.diff);
	s_w8(&s, BPM_ERM_FRAMES_OUTPUT);
	s_wb32(&s, m_track->rendition_frames_output.diff);

	m_track->sei_rendered[BPM_ERM_SEI] = true;
}

/* Note : extract_buffer_from_sei() and nal_start are also defined
 * in obs-output.c, however they are not public APIs. When the caption
 * library is re-worked, this code should be refactored into that.
 */
static size_t extract_buffer_from_sei(sei_t *sei, uint8_t **data_out)
{
	if (!sei || !sei->head) {
		return 0;
	}
	/* We should only need to get one payload, because the SEI that was
	 * generated should only have one message, so no need to iterate. If
	 * we did iterate, we would need to generate multiple OBUs. */
	sei_message_t *msg = sei_message_head(sei);
	int payload_size = (int)sei_message_size(msg);
	uint8_t *payload_data = sei_message_data(msg);
	*data_out = bmalloc(payload_size);
	memcpy(*data_out, payload_data, payload_size);
	return payload_size;
}
static const uint8_t nal_start[4] = {0, 0, 0, 1};

/* process_metrics() will update and insert unregistered
 * SEI (AVC/HEVC) or OBU (AV1) messages into the encoded
 * video bitstream.
*/
static bool process_metrics(obs_output_t *output, struct encoder_packet *out,
			    struct encoder_packet_time *ept,
			    struct metrics_data *m_track)
{
	struct encoder_packet backup = *out;
	sei_t sei;
	uint8_t *data = NULL;
	size_t size;
	long ref = 1;
	bool avc = false;
	bool hevc = false;
	bool av1 = false;

	if (!m_track) {
		blog(LOG_DEBUG,
		     "Metrics track for index: %lu had not be initialized",
		     out->track_idx);
		return false;
	}

	// Update the metrics for this track
	if (!update_metrics(output, out, ept, m_track)) {
		// Something went wrong; log it and return
		blog(LOG_DEBUG, "update_metrics() for track index: %lu failed",
		     out->track_idx);
		return false;
	}

	if (strcmp(obs_encoder_get_codec(out->encoder), "h264") == 0) {
		avc = true;
	} else if (strcmp(obs_encoder_get_codec(out->encoder), "av1") == 0) {
		av1 = true;
#ifdef ENABLE_HEVC
	} else if (strcmp(obs_encoder_get_codec(out->encoder), "hevc") == 0) {
		hevc = true;
#endif
	}

#ifdef ENABLE_HEVC
	uint8_t hevc_nal_header[2];
	if (hevc) {
		size_t nal_header_index_start = 4;
		// Skip past the annex-b start code
		if (memcmp(out->data, nal_start + 1, 3) == 0) {
			nal_header_index_start = 3;
		} else if (memcmp(out->data, nal_start, 4) == 0) {
			nal_header_index_start = 4;

		} else {
			/* We shouldn't ever see this unless we start getting
			 * packets without annex-b start codes. */
			blog(LOG_DEBUG,
			     "Annex-B start code not found, we may not "
			     "generate a valid hevc nal unit header "
			     "for our caption");
			return false;
		}
		/* We will use the same 2 byte NAL unit header for the SEI,
		 * but swap the NAL types out. */
		hevc_nal_header[0] = out->data[nal_header_index_start];
		hevc_nal_header[1] = out->data[nal_header_index_start + 1];
	}
#endif
	// Create array for the original packet data + the SEI appended data
	DARRAY(uint8_t) out_data;
	da_init(out_data);

	// Copy the original packet
	da_push_back_array(out_data, (uint8_t *)&ref, sizeof(ref));
	da_push_back_array(out_data, out->data, out->size);

	// Build the SEI metrics message payload
	bpm_ts_sei_render(m_track);
	bpm_sm_sei_render(m_track);
	bpm_erm_sei_render(m_track);

	// Iterate over all the BPM SEI types
	for (uint8_t i = 0; i < BPM_MAX_SEI; ++i) {
		// Create and inject the syntax specific SEI messages in the bitstream if the rendering was successful
		if (m_track->sei_rendered[i]) {
			// Send one SEI message per NALU or OBU
			sei_init(&sei, 0.0);

			// Generate the formatted SEI message
			sei_message_t *msg = sei_message_new(
				sei_type_user_data_unregistered,
				m_track->sei_payload[i].bytes.array,
				m_track->sei_payload[i].bytes.num);
			sei_message_append(&sei, msg);

			// Free the SEI payload buffer in the metrics track
			array_output_serializer_free(&m_track->sei_payload[i]);

			// Update for any codec specific syntax and add to the output bitstream
			if (avc || hevc || av1) {
				if (avc || hevc) {
					data = bmalloc(sei_render_size(&sei));
					size = sei_render(&sei, data);
				}
				/* In each of these specs there is an identical structure that
				 * carries user private metadata. We have an AVC SEI wrapped
				 * version of that here. We will strip it out and repackage
				 * it slightly to fit the different codec carrying mechanisms.
				 * A slightly modified SEI for HEVC and a metadata OBU for AV1.
				 */
				if (avc) {
					/* TODO: SEI should come after AUD/SPS/PPS,
					 * but before any VCL */
					da_push_back_array(out_data, nal_start,
							   4);
					da_push_back_array(out_data, data,
							   size);
#ifdef ENABLE_HEVC
				} else if (hevc) {
					/* Only first NAL (VPS/PPS/SPS) should use the 4 byte
					 * start code. SEIs use 3 byte version */
					da_push_back_array(out_data,
							   nal_start + 1, 3);
					/* nal_unit_header( ) {
					 * forbidden_zero_bit       f(1)
					 * nal_unit_type            u(6)
					 * nuh_layer_id             u(6)
					 * nuh_temporal_id_plus1    u(3)
					 * }
					 */
					const uint8_t prefix_sei_nal_type = 39;
					/* The first bit is always 0, so we just need to
					 * save the last bit off the original header and
					 * add the SEI NAL type. */
					uint8_t first_byte =
						(prefix_sei_nal_type << 1) |
						(0x01 & hevc_nal_header[0]);
					hevc_nal_header[0] = first_byte;
					/* The HEVC NAL unit header is 2 byte instead of
					 * one, otherwise everything else is the
					 * same. */
					da_push_back_array(out_data,
							   hevc_nal_header, 2);
					da_push_back_array(out_data, &data[1],
							   size - 1);
#endif
				} else if (av1) {
					uint8_t *obu_buffer = NULL;
					size_t obu_buffer_size = 0;
					size = extract_buffer_from_sei(&sei,
								       &data);
					metadata_obu(
						data, size, &obu_buffer,
						&obu_buffer_size,
						METADATA_TYPE_USER_PRIVATE_6);
					if (obu_buffer) {
						da_push_back_array(
							out_data, obu_buffer,
							obu_buffer_size);
						bfree(obu_buffer);
					}
				}
				if (data) {
					bfree(data);
				}
			}
			sei_free(&sei);
		}
	}
	obs_encoder_packet_release(out);

	*out = backup;
	out->data = (uint8_t *)out_data.array + sizeof(ref);
	out->size = out_data.num - sizeof(ref);

	if (avc || hevc || av1) {
		return true;
	}
	return false;
}

static struct metrics_data *bpm_create_metrics_track(void)
{
	struct metrics_data *rval = bzalloc(sizeof(struct metrics_data));
	pthread_mutex_init_value(&rval->metrics_mutex);

	if (pthread_mutex_init(&rval->metrics_mutex, NULL) != 0) {
		bfree(rval);
		rval = NULL;
	}
	return rval;
}

static bool bpm_get_track(obs_output_t *output, size_t track,
			  struct metrics_data **m_track)
{
	bool found = false;
	// Walk the DARRAY looking for the output pointer
	pthread_mutex_lock(&bpm_metrics_mutex);
	for (size_t i = bpm_metrics.num; i > 0; i--) {
		if (output == bpm_metrics.array[i - 1].output) {
			*m_track =
				bpm_metrics.array[i - 1].metrics_tracks[track];
			found = true;
			break;
		}
	}

	if (!found) {
		// Create the new BPM metrics entries
		struct output_metrics_link *oml = da_push_back_new(bpm_metrics);

		oml->output = output;
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; ++i) {
			oml->metrics_tracks[i] = bpm_create_metrics_track();
		}
		*m_track = oml->metrics_tracks[track];
		found = true;
	}
	pthread_mutex_unlock(&bpm_metrics_mutex);
	return found;
}

void bpm_destroy(obs_output_t *output)
{
	int64_t idx = -1;

	pthread_mutex_lock(&bpm_metrics_mutex);

	// Walk the DARRAY looking for the index that matches the output
	for (size_t i = bpm_metrics.num; i > 0; i--) {
		if (output == bpm_metrics.array[i - 1].output) {
			idx = i - 1;
			break;
		}
	}
	if (idx >= 0) {
		struct output_metrics_link *oml = &bpm_metrics.array[idx];
		for (size_t i = 0; i < MAX_OUTPUT_VIDEO_ENCODERS; i++) {
			if (oml->metrics_tracks[i]) {
				struct metrics_data *m_track =
					oml->metrics_tracks[i];
				for (uint8_t j = 0; j < BPM_MAX_SEI; ++j) {
					array_output_serializer_free(
						&m_track->sei_payload[j]);
				}
				pthread_mutex_destroy(&m_track->metrics_mutex);
				bfree(m_track);
				m_track = NULL;
			}
		}
		da_erase(bpm_metrics, idx);
		if (bpm_metrics.num == 0)
			da_free(bpm_metrics);
	}
	pthread_mutex_unlock(&bpm_metrics_mutex);
}

static void bpm_init(void)
{
	pthread_mutex_init_value(&bpm_metrics_mutex);
	da_init(bpm_metrics);
}

/* bpm_inject() is the callback function that needs to be registered
 * with each output needing Broadcast Performance Metrics injected
 * into the video bitstream, using SEI (AVC/HEVC) and OBU (AV1) syntax.
 */
void bpm_inject(obs_output_t *output, struct encoder_packet *pkt,
		struct encoder_packet_time *pkt_time, void *param)
{
	UNUSED_PARAMETER(param);

	static pthread_once_t once = PTHREAD_ONCE_INIT;
	pthread_once(&once, bpm_init);

	if (!output || !pkt) {
		blog(LOG_DEBUG,
		     "%s: Null pointer arguments supplied, returning",
		     __FUNCTION__);
		return;
	}

	/* Insert BPM on video frames and only when a keyframe
	 * is detected.
	 */
	if (pkt->type == OBS_ENCODER_VIDEO && pkt->keyframe) {
		/* Video packet must have pkt_timing supplied for BPM */
		if (!pkt_time) {
			blog(LOG_DEBUG,
			     "%s: Packet timing missing for track %ld, PTS %" PRId64,
			     __FUNCTION__, pkt->track_idx, pkt->pts);
			return;
		}

		/* Get the metrics track associated with the output.
		 * Allocate BPM metrics structures for the output if needed.
		 */
		struct metrics_data *m_track = NULL;
		if (!bpm_get_track(output, pkt->track_idx, &m_track)) {
			blog(LOG_DEBUG, "%s: BPM metrics track not found!",
			     __FUNCTION__);
			return;
		}

		pthread_mutex_lock(&m_track->metrics_mutex);
		/* Update the metrics and generate BPM messages. */
		if (!process_metrics(output, pkt, pkt_time, m_track)) {
			blog(LOG_DEBUG, "%s: BPM injection processing failed",
			     __FUNCTION__);
		}
		pthread_mutex_unlock(&m_track->metrics_mutex);
	}
}
