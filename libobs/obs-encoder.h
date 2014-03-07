/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#pragma once

/** Specifies the encoder type */
enum obs_encoder_type {
	OBS_PACKET_AUDIO,
	OBS_PACKET_VIDEO
};

/** Encoder output packet */
struct encoder_packet {
	uint8_t               *data;      /**< Packet data */
	size_t                size;       /**< Packet size */

	int64_t               pts;        /**< Presentation timestamp */
	int64_t               dts;        /**< Decode timestamp */

	enum obs_encoder_type type;       /**< Encoder type */

	/**
	 * Packet priority
	 *
	 * This is generally use by video encoders to specify the priority
	 * of the packet.  If this frame is dropped, it will have to wait for
	 * another packet of drop_priority.
	 */
	int                   priority;

	/**
	 * Dropped packet priority
	 *
	 * If this packet is dropped, the next packet must be of this priority
	 * or higher to continue transmission.
	 */
	int                   drop_priority;
};

/** Encoder input frame */
struct encoder_frame {
	/** Data for the frame/audio */
	uint8_t               *data[MAX_AV_PLANES];

	/** size of each plane */
	uint32_t              linesize[MAX_AV_PLANES];

	/** Number of frames (audio only) */
	uint32_t              frames;

	/** Presentation timestamp */
	int64_t               pts;
};

/**
 * Encoder interface
 *
 * Encoders have a limited usage with OBS.  You are not generally supposed to
 * implement every encoder out there.  Generally, these are limited or specific
 * encoders for h264/aac for streaming and recording.  It doesn't have to be
 * *just* h264 or aac of course, but generally those are the expected encoders.
 *
 * That being said, other encoders will be kept in mind for future use.
 */
struct obs_encoder_info {
	/* ----------------------------------------------------------------- */
	/* Required implementation*/

	/** Specifies the named identifier of this encoder */
	const char *id;

	/**
	 * Gets the full translated name of this encoder
	 *
	 * @param  locale  Locale to use for translation
	 * @return         Translated name of the encoder
	 */
	const char *(*getname)(const char *locale);

	/**
	 * Creates the encoder with the specified settings
	 *
	 * @param  settings  Settings for the encoder
	 * @param  encoder   OBS encoder context
	 * @return           Data associated with this encoder context
	 */
	void *(*create)(obs_data_t settings, obs_encoder_t encoder);

	/**
	 * Destroys the encoder data
	 *
	 * @param  data  Data associated with this encoder context
	 */
	void (*destroy)(void *data);

	/**
	 * Resets the encoder with the specified settings
	 *
	 * @param  data      Data associated with this encoder context
	 * @param  settings  New settings for the encoder
	 * @return           true if successful, false otherwise
	 */
	bool (*reset)(void *data, obs_data_t settings);

	/**
	 * Encodes frame(s), and outputs encoded packets as they become
	 * available.
	 *
	 * @param       data             Data associated with this encoder
	 *                               context
	 * @param[in]   frame            Raw audio/video data to encode
	 * @param[out]  packet           Encoder packet output, if any
	 * @param[out]  received_packet  Set to true if a packet was received,
	 *                               false otherwise
	 * @return                       true if successful, false otherwise.
	 */
	int (*encode)(void *data, const struct encoder_frame *frame,
			struct encoder_packet *packet, bool *received_packet);

	/* ----------------------------------------------------------------- */
	/* Optional implementation */

	/**
	 * Gets the default settings for this encoder
	 *
	 * @param[out]  settings  Data to assign default settings to
	 */
	void (*defaults)(obs_data_t settings);

	/** 
	 * Gets the property information of this encoder
	 *
	 * @param  locale  The locale to translate with
	 * @return         The properties data
	 */
	obs_properties_t (*properties)(const char *locale);

	/**
	 * Updates the settings for this encoder
	 *
	 * @param  data      Data associated with this encoder context
	 * @param  settings  New settings for this encoder
	 */
	void (*update)(void *data, obs_data_t settings);

	/**
	 * Returns extra data associated with this encoder (usually header)
	 *
	 * @param  data        Data associated with this encoder context
	 * @param  extra_data  Pointer to receive the extra data
	 * @param  size        Pointer to receive the size of the extra data
	 */
	bool (*get_extra_data)(void *data, uint8_t **extra_data, size_t *size);
};

/**
 * Register an encoder definition to the current obs context.  This should be
 * used in obs_module_load.
 *
 * @param  info  Pointer to the source definition structure.
 */
EXPORT void obs_register_encoder(const struct obs_encoder_info *info);
