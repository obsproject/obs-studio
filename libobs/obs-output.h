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
 *       + myoutput_update
 *       + myoutput_start
 *       + myoutput_stop
 *       + myoutput_active
 *
 *       [and optionally]
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
 *   bool [name]_start(void *data)
 *       Starts output
 *
 *       Return value: true if successful
 *
 * ---------------------------------------------------------
 *   void [name]_stop(void *data)
 *       Stops output
 *
 * ---------------------------------------------------------
 *   bool [name]_active(void *data)
 *       Returns whether currently active or not
 *
 * ===========================================
 *   Optional Output Exports
 * ===========================================
 *   void [name]_pause(void *data)
 *       Pauses output.  Typically only usable for local recordings.
 */

struct obs_output;

struct output_info {
	const char *id;

	const char *(*getname)(const char *locale);

	void *(*create)(const char *settings, struct obs_output *output);
	void (*destroy)(void *data);

	void (*update)(void *data, const char *settings);

	bool (*start)(void *data);
	void (*stop)(void *data);

	bool (*active)(void *data);

	/* optional */
	void (*pause)(void *data);
};

struct obs_output {
	char               *name;
	void               *data;
	struct output_info callbacks;
	struct dstr        settings;
};

extern bool load_output_info(void *module, const char *module_name,
		const char *output_name, struct output_info *info);
