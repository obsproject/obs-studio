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

struct obs_encoder_info {
	const char *id;

	const char *(*getname)(const char *locale);

	void *(*create)(obs_data_t settings, obs_encoder_t encoder);
	void (*destroy)(void *data);

	bool (*reset)(void *data);

	int (*encode)(void *data, void *frames, size_t size,
			struct encoder_packet **packets);
	int (*getheader)(void *data, struct encoder_packet **packets);

	/* optional */
	void (*update)(void *data, obs_data_t settings);

	obs_properties_t (*properties)(const char *locale);

	bool (*setbitrate)(void *data, uint32_t bitrate, uint32_t buffersize);
	bool (*request_keyframe)(void *data);
};

EXPORT void obs_register_encoder(const struct obs_encoder_info *info);
