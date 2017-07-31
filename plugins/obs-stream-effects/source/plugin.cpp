/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "plugin.h"
#include "filter-blur.h"
#include "filter-displacement.h"
#include "filter-shape.h"
#include "filter-transform.h"

OBS_DECLARE_MODULE();
OBS_MODULE_AUTHOR("Michael Fabian Dirks");
OBS_MODULE_USE_DEFAULT_LOCALE("obs-stream-effects", "en-US");

Filter::Blur *filterBlur;
Filter::Displacement *filterDisplacement;
Filter::Shape *filterShape;
Filter::Transform *filterTransform;

MODULE_EXPORT bool obs_module_load(void) {
	filterDisplacement = new Filter::Displacement();
	filterShape = new Filter::Shape();
	filterTransform = new Filter::Transform();
	filterBlur = new Filter::Blur();
	return true;
}

MODULE_EXPORT void obs_module_unload(void) {
	delete filterTransform;
	delete filterShape;
	delete filterDisplacement;
	delete filterBlur;
}

MODULE_EXPORT const char* obs_module_name() {
	return PLUGIN_NAME;
}

MODULE_EXPORT const char* obs_module_description() {
	return PLUGIN_NAME;
}

#ifdef _WIN32
#define NOMINMAX
#define NOINOUT

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
	return TRUE;
}
#endif

