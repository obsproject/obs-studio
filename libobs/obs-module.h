/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs.h"

#ifdef __cplusplus
#define MODULE_EXPORT extern "C" EXPORT
#else
#define MODULE_EXPORT EXPORT
#endif

/**
 * @file
 *
 * This file is used by modules for module declaration and module exports.
 */

/** Required: Declares a libobs module. */
#define OBS_DECLARE_MODULE()                                          \
	MODULE_EXPORT uint32_t obs_module_ver(void);                  \
	uint32_t obs_module_ver(void) {return LIBOBS_API_VER;}

/**
 * Required: Called when the module is loaded.  Use this function to load all
 * the sources/encoders/outputs/services for your module, or anything else that
 * may need loading.
 *
 * @param libobs_ver The version of libobs.
 * @return           Return true to continue loading the module, otherwise
 *                   false to indcate failure and unload the module
 */
MODULE_EXPORT bool obs_module_load(uint32_t libobs_version);

/** Optional: Called when the module is unloaded.  */
MODULE_EXPORT void obs_module_unload(void);

/** Called to set the current locale data for the module.  */
MODULE_EXPORT void obs_module_set_locale(const char *locale);

/**
 * Optional: Declares the author(s) of the module
 *
 * @param name Author name(s)
 */
#define OBS_MODULE_AUTHOR(name) \
	MODULE_EXPORT const char *obs_module_author(void); \
	const char *obs_module_author(void) {return name;}

/** Optional: Returns a description of the module */
MODULE_EXPORT const char *obs_module_description(void);
