/******************************************************************************
    Copyright (C) 2026-2026 by Adam Fallon <adam.eric.fallon@gmail.com>

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

#include "obs-internal.h"
#include "obs-hotkey.h"
#include <glib.h>
#include <stdbool.h>

// must be called before registering any hotkeys with the portal.
// does nothing if it has already been called
void obs_hotkey_portal_init();
void obs_hotkey_portal_free();

// returns true if the portal session has been started
bool obs_hotkey_portal_session_active();

// queues hotkey to be added to portal
// should always be called, even if it has no effect (i.e. if unsupported on
// the current platform)
void obs_hotkey_portal_register(obs_hotkey_t *hotkey);

// stops listening to a hotkey
// should always be called, even if it has no effect (i.e. if unsupported on
// the current platform)
void obs_hotkey_portal_unregister(obs_hotkey_id id);
