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
#include <obs-module.h>
#include <obs-nix-platform.h>
#include "xcomposite-input.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("linux-xshm", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "xcomposite/xshm based window/screen capture for X11";
}

extern struct obs_source_info xshm_input;

bool obs_module_load(void)
{
	enum obs_nix_platform_type platform = obs_get_nix_platform();

	switch (platform) {
	case OBS_NIX_PLATFORM_X11_EGL:
		obs_register_source(&xshm_input);
		xcomposite_load();
		break;

#ifdef ENABLE_WAYLAND
	case OBS_NIX_PLATFORM_WAYLAND:
		break;
#endif
	}

	return true;
}

void obs_module_unload(void)
{
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_X11_EGL)
		xcomposite_unload();
}
