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
 *  Outputs 
 * ===========================================
 *
 *   An output takes raw audio and/or video and processes and/or outputs it
 * to a destination, whether that destination be a file, network, or other.
 *
 *   A module with outputs needs to export these functions:
 *       + enum_outputs
 *
 *   Each individual output is then exported by it's name.  For example, an
 * output named "myoutput" would have the following exports:
 *       + myoutput_getname
 *       + myoutput_create
 *       + myoutput_destroy
 *       + myoutput_start
 *       + myoutput_stop
 *       + myoutput_encoders
 *
 *       [and optionally]
 *       + myoutput_setencoder
 *       + myoutput_getencoder
 *       + myoutput_config
 *       + myoutput_pause
 *
 * ===========================================
 *   Primary Exports
 * ===========================================
 *   const char *enum_outputs(size_t idx);
 *       idx: index of the output.
 *       Return value: Output identifier name.  NULL when no more available.
 *
 * ===========================================
 *   Output Exports
 * ===========================================
 *   const char *[name]_getname(const char *locale);
 *       Returns the full translated name of the output type (seen by the user).
 *
 * ---------------------------------------------------------
 *   void *[name]_create(const char *settings, obs_output_t output);
 *       Creates an output.
 *
 *       settings: Settings of the output.
 *       output: pointer to main output
 *       Return value: Internal output pointer, or NULL if failed.
 *
 * ---------------------------------------------------------
 *   void [name]_destroy(void *data);
 *       Destroys the output.
 *
 * ---------------------------------------------------------
 *   void [name]_update(void *data, const char *settings)
 *       Updates the output's settings
 *
 *       settings: New settings of the output
 *
 * ---------------------------------------------------------
 *   bool [name]_reset(void *data)
 *       Starts output
 *
 *       Return value: true if successful
 *
 * ---------------------------------------------------------
 *   int [name]_encode(void *data, void *frames, size_t size,
 *                      struct encoder_packet **packets)
 *
 *       frames: frame data
 *       size: size of data pointed to by the frame parameter
 *       packets: returned packets, or NULL if none
 *       Return value: number of output frames
 *
 * ---------------------------------------------------------
 *   bool [name]_reset(void *data)
 *       Resets encoder data
 *
 *       Return value: true if successful
 *
 * ===========================================
 *   Optional Output Exports
 * ===========================================
 *   void [name]_setbitrate(void *data, uint32_t bitrate, uint32_t buffersize);
 *       Sets the bitrate of the encoder
 */

struct obs_encoder;

struct encoder_info {
	const char *id;

	const char *(*getname)(const char *locale);

	void *(*create)(const char *settings, struct obs_encoder *encoder);
	void (*destroy)(void *data);

	void (*update)(void *data, const char *settings);

	bool (*reset)(void *data);

	int (*encode)(void *data, void *frames, size_t size,
			struct encoder_packet **packets);
	int (*getheader)(void *data, struct encoder_packet **packets);

	/* optional */
	void (*setbitrate)(void *data, uint32_t bitrate, uint32_t buffersize);
	void (*request_keyframe)(void *data);
};

struct obs_encoder_callback {
	void (*new_packet)(void *param, struct encoder_packet *packet);
	void *param;
};

struct obs_encoder {
	char                                *name;
	void                                *data;
	struct encoder_info                 callbacks;
	struct dstr                         settings;

	pthread_mutex_t                     data_callbacks_mutex;
	DARRAY(struct obs_encoder_callback) data_callbacks;
};

extern bool load_encoder_info(void *module, const char *module_name,
		const char *encoder_name, struct encoder_info *info);
