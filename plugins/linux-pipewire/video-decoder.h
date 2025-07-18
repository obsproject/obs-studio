/* camera-portal.c
 *
 * Copyright 2021 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <obs-module.h>

typedef struct _obs_pipewire_decoder obs_pipewire_decoder;

/**
 * Initialize the decoder.
 * The decoder must be destroyed on failure.
 *
 * @param decoder the decoder structure
 * @param subtype which codec is used
 * @return non-zero on failure
 */
obs_pipewire_decoder *obs_pipewire_decoder_new(uint32_t subtype);

/**
 * Free any data associated with the decoder.
 *
 * @param decoder the decoder structure
 */
void obs_pipewire_decoder_destroy(obs_pipewire_decoder *decoder);

/**
 * Decode a jpeg or h264 frame into an obs frame
 *
 * @param out the obs frame to decode into
 * @param data the codec data
 * @param length length of the data
 * @param decoder the decoder as initialized by pipewire_init_decoder
 * @return non-zero on failure
 */
int obs_pipewire_decoder_decode_frame(obs_pipewire_decoder *decoder, struct obs_source_frame *out, uint8_t *data,
				      size_t length);
