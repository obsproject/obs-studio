// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

#include "dacast/dacast.h"
#include "nimotv/nimotv.h"
#include "showroom/showroom.h"
#include "younow/younow.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("orphaned-services", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Services not maintained by OBS Project";
}

bool obs_module_load(void)
{
	load_dacast();
	load_nimotv();
	load_showroom();
	load_younow();
	return true;
}

void obs_module_unload(void)
{
	unload_dacast();
	unload_showroom();
}
