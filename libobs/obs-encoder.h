/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "util/c99defs.h"
#include "util/dstr.h"

/*
 * ===========================================
 *  Encoders 
 * ===========================================
 *
 *   An encoder context allows data to be encoded from raw output, and allow
 * it to be used to output contexts (such as outputting to stream).
 *
 *   A module with encoders needs to export these functions:
 *       + enum_encoders
 *
 *   Each individual encoder is then exported by it's name.  For example, an
 * encoder named "myencoder" would have the following exports:
 *       + myencoder_getname
 *       + myencoder_create
 *       + myencoder_destroy
 *       + myencoder_update
 *       + myencoder_reset
 *       + myencoder_encode
 *       + myencoder_getheader
 *
 *       [and optionally]
 *       + myencoder_setbitrate
 *       + myencoder_request_keyframe
 *
 * ===========================================
 *   Primary Exports
 * ===========================================
 *   const char *enum_encoders(size_t idx);
 *       idx: index of the encoder.
 *       Return value: Encoder identifier name.  NULL when no more available.
 *
 * ===========================================
 *   Encoder Exports
 * ===========================================
 *   const char *[name]_getname(const char *locale);
 *       Returns the full translated name of the encoder type
 *       (seen by the user).
 *
 * ---------------------------------------------------------
 *   void *[name]_create(obs_data_t settings, const char *name,
 *                       obs_encoder_t encoder);
 *       Creates an encoder.
 *
 *       settings: Settings of the encoder.
 *       name: Name of the encoder.
 *       encoder: Pointer to encoder context.
 *       Return value: Internal encoder pointer, or NULL if failed.
 *
 * ---------------------------------------------------------
 *   void [name]_destroy(void *data);
 *       Destroys the encoder.
 *
 * ---------------------------------------------------------
 *   void [name]_update(void *data, obs_data_t settings)
 *       Updates the encoder's settings
 *
 *       settings: New settings of the encoder
 *
 * ---------------------------------------------------------
 *   bool [name]_reset(void *data)
 *       Restarts encoder
 *
 *       Return value: true if successful
 *
 * ---------------------------------------------------------
 *   int [name]_encode(void *data, void *frames, size_t size,
 *                      struct encoder_packet **packets)
 *       Encodes data.
 *
 *       frames: frame data
 *       size: size of data pointed to by the frame parameter
 *       packets: returned packets, or NULL if none
 *       Return value: number of encoder frames
 *
 * ---------------------------------------------------------
 *   int [name]_getheader(void *data, struct encoder_packet **packets)
 *       Returns the header packets for this encoder.
 *
 *       packets: returned packets, or NULL if none
 *       Return value: number of encoder frames
 *
 * ===========================================
 *   Optional Encoder Exports
 * ===========================================
 *   bool [name]_setbitrate(void *data, uint32_t bitrate, uint32_t buffersize);
 *       Sets the bitrate of the encoder
 *
 *       bitrate: Bitrate
 *       buffersize: Buffer size
 *       Returns true if successful/compatible
 *
 * ---------------------------------------------------------
 *   bool [name]_request_keyframe(void *data)
 *       Requests a keyframe from the encoder
 *
 *       Returns true if successful/compatible.
 */

struct obs_encoder;

struct encoder_info {
	const char *id;

	const char *(*getname)(const char *locale);

	void *(*create)(obs_data_t settings, struct obs_encoder *encoder);
	void (*destroy)(void *data);

	void (*update)(void *data, obs_data_t settings);

	bool (*reset)(void *data);

	int (*encode)(void *data, void *frames, size_t size,
			struct encoder_packet **packets);
	int (*getheader)(void *data, struct encoder_packet **packets);

	/* optional */
	bool (*setbitrate)(void *data, uint32_t bitrate, uint32_t buffersize);
	bool (*request_keyframe)(void *data);
};

struct obs_encoder_callback {
	void (*new_packet)(void *param, struct encoder_packet *packet);
	void *param;
};

struct obs_encoder {
	char                                *name;
	void                                *data;
	struct encoder_info                 callbacks;
	obs_data_t                          settings;

	pthread_mutex_t                     data_callbacks_mutex;
	DARRAY(struct obs_encoder_callback) data_callbacks;
};

extern bool load_encoder_info(void *module, const char *module_name,
		const char *encoder_name, struct encoder_info *info);
