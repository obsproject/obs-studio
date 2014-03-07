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

struct obs_output_info {
	/* required */
	const char *id;

	const char *(*getname)(const char *locale);

	void *(*create)(obs_data_t settings, obs_output_t output);
	void (*destroy)(void *data);

	bool (*start)(void *data);
	void (*stop)(void *data);

	bool (*active)(void *data);

	/* optional */
	void (*update)(void *data, obs_data_t settings);

	void (*defaults)(obs_data_t settings);

	obs_properties_t (*properties)(const char *locale);

	void (*pause)(void *data);
};

EXPORT void obs_register_output(const struct obs_output_info *info);
