/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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
*/

#pragma once

#include "obs.h"

/**
 * @file
 * @brief header for audio controls
 *
 * @brief Audio controls for use in GUIs
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fader types
 */
enum obs_fader_type {
	/**
	 * @brief A simple cubic fader for controlling audio levels
	 *
	 * This is a very common type of software fader since it yields good
	 * results while being quite performant.
	 * The input value is mapped to mul values with the simple formula x^3.
	 */
	OBS_FADER_CUBIC
};

/**
 * @brief Create a fader
 * @param type the type of the fader
 * @return pointer to the fader object
 *
 * A fader object is used to map input values from a gui element to dB and
 * subsequently multiplier values used by libobs to mix audio.
 * The current "position" of the fader is internally stored as dB value.
 */
EXPORT obs_fader_t *obs_fader_create(enum obs_fader_type type);

/**
 * @brief Destroy a fader
 * @param fader pointer to the fader object
 *
 * Destroy the fader and free all related data
 */
EXPORT void obs_fader_destroy(obs_fader_t *fader);

/**
 * @brief Set the fader dB value
 * @param fader pointer to the fader object
 * @param db new dB value
 * @return true if value was set without clamping
 */
EXPORT bool obs_fader_set_db(obs_fader_t *fader, const float db);

/**
 * @brief Get the current fader dB value
 * @param fader pointer to the fader object
 * @return current fader dB value
 */
EXPORT float obs_fader_get_db(obs_fader_t *fader);

/**
 * @brief Set the fader value from deflection
 * @param fader pointer to the fader object
 * @param def new deflection
 * @return true if value was set without clamping
 *
 * This sets the new fader value from the supplied deflection, in case the
 * resulting value was clamped due to limits this function will return false.
 * The deflection is typically in the range [0.0, 1.0] but may be higher in
 * order to provide some amplification. In order for this to work the high dB
 * limit has to be set.
 */
EXPORT bool obs_fader_set_deflection(obs_fader_t *fader, const float def);

/**
 * @brief Get the current fader deflection
 * @param fader pointer to the fader object
 * @return current fader deflection
 */
EXPORT float obs_fader_get_deflection(obs_fader_t *fader);

/**
 * @brief Set the fader value from multiplier
 * @param fader pointer to the fader object
 * @return true if the value was set without clamping
 */
EXPORT bool obs_fader_set_mul(obs_fader_t *fader, const float mul);

/**
 * @brief Get the current fader multiplier value
 * @param fader pointer to the fader object
 * @return current fader multiplier
 */
EXPORT float obs_fader_get_mul(obs_fader_t *fader);

/**
 * @brief Attach the fader to a source
 * @param fader pointer to the fader object
 * @param source pointer to the source object
 * @return true on success
 *
 * When the fader is attached to a source it will automatically sync it's state
 * to the volume of the source.
 */
EXPORT bool obs_fader_attach_source(obs_fader_t *fader, obs_source_t *source);

/**
 * @brief Detach the fader from the currently attached source
 * @param fader pointer to the fader object
 */
EXPORT void obs_fader_detach_source(obs_fader_t *fader);

/**
 * @brief Get signal handler for the fader
 * @param fader pointer to the fader object
 * @return signal handler
 */
EXPORT signal_handler_t *obs_fader_get_signal_handler(obs_fader_t *fader);

#ifdef __cplusplus
}
#endif
