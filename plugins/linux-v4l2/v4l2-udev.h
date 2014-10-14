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

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize udev system to watch for device events
 *
 * @return monitor object, or NULL on error
 */
void *v4l2_init_udev(void);

/**
 * Unref the udev system
 *
 * This will also remove any registered callbacks if there are any
 *
 * @param monitor monitor object
 */
void v4l2_unref_udev(void *monitor);

/**
 * Callback when a device was added.
 *
 * @param dev device node of the device that was added
 * @param userdata pointer to userdata specified when registered
 */
typedef void (*v4l2_device_added_cb)(const char *dev, void *userdata);

/**
 * Callback when a device was removed.
 *
 * @param dev device node of the device that was removed
 * @param userdata pointer to userdata specified when registered
 */
typedef void (*v4l2_device_removed_cb)(const char *dev, void *userdata);

/**
 * Register the device added callback
 *
 * @param monitor monitor object
 * @param cb the function that should be called
 * @param userdata pointer to userdata that should be passed to the callback
 */
void v4l2_set_device_added_callback(void *monitor, v4l2_device_added_cb cb,
		void *userdata);

/**
 * Register the device remove callback
 *
 * @param monitor monitor object
 * @param cb the function that should be called
 * @param userdata pointer to userdata that should be passed to the callback
 */
void v4l2_set_device_removed_callback(void *monitor, v4l2_device_removed_cb cb,
		void *userdata);

#ifdef __cplusplus
}
#endif
