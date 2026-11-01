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

#pragma once

#include <obs.h>
#include <util/serializer.h>

struct mp4_mux;

/* Flavor for target compatibility */
enum mp4_flavor {
	FLAVOR_MP4,  /* ISO/IEC 14496-12 */
	FLAVOR_MOV,  /* Apple QuickTime */
	FLAVOR_CMAF, /* ISO/IEC 23000-19 (not yet implemented) */
};

enum mp4_mux_flags {
	/* Uses mdta key/value list for metadata instead of QuickTime keys */
	MP4_USE_MDTA_KEY_VALUE = 1 << 0,
	/* Write encoder configuration to trak udat */
	MP4_WRITE_ENCODER_INFO = 1 << 1,
	/* Skip "soft-remux" and leave file in fragmented state */
	MP4_SKIP_FINALISATION = 1 << 2,
	/* Use negative CTS instead of edit lists */
	MP4_USE_NEGATIVE_CTS = 1 << 3,
};

struct mp4_mux *mp4_mux_create(obs_output_t *output, struct serializer *serializer, enum mp4_mux_flags flags,
			       enum mp4_flavor flavor);
void mp4_mux_destroy(struct mp4_mux *mux);
bool mp4_mux_submit_packet(struct mp4_mux *mux, struct encoder_packet *pkt);
bool mp4_mux_add_chapter(struct mp4_mux *mux, int64_t dts_usec, const char *name);
bool mp4_mux_finalise(struct mp4_mux *mux);
