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

#define OBS_SIZE_FUNC(structure, func)                       \
	MODULE_EXPORT size_t func(void);                     \
	size_t func(void) {return sizeof(struct structure);}

/**
 * @file
 *
 * This file is used by modules for module declaration and module exports.
 */

/** Required: Declares a libobs module. */
#define OBS_DECLARE_MODULE()                                          \
	MODULE_EXPORT uint32_t obs_module_ver(void);                  \
	uint32_t obs_module_ver(void) {return LIBOBS_API_VER;}        \
	OBS_SIZE_FUNC(obs_source_info,  obs_module_source_info_size)  \
	OBS_SIZE_FUNC(obs_output_info,  obs_module_output_info_size)  \
	OBS_SIZE_FUNC(obs_encoder_info, obs_module_encoder_info_size) \
	OBS_SIZE_FUNC(obs_encoder_info, obs_module_service_info_size) \
	OBS_SIZE_FUNC(obs_modal_ui,     obs_module_modal_ui_size)     \
	OBS_SIZE_FUNC(obs_modeless_ui,  obs_module_modeless_ui_size)

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

/**
 * Optional: Declares the author(s) of the module
 *
 * @param name Author name(s)
 */
#define OBS_MODULE_AUTHOR(name) \
	MODULE_EXPORT const char *obs_module_author(void); \
	const char *obs_module_author(void) {return name;}

/**
 * Optional: Declares the author of the module
 *
 * @param locale Locale to look up the description for.
 */
MODULE_EXPORT const char *obs_module_description(const char *locale);
