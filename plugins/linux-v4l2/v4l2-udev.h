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

#include <callback/signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize udev system to watch for device events
 */
void v4l2_init_udev(void);

/**
 * Unref the udev system
 */
void v4l2_unref_udev(void);

/**
 * Get signal handler
 *
 * @return signal handler for udev events
 */
signal_handler_t *v4l2_get_udev_signalhandler(void);

#ifdef __cplusplus
}
#endif
