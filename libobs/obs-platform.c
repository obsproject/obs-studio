/******************************************************************************
    Copyright (C) 2019 by Jason Francis <cycl0ps@tuta.io>

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

#include "obs-platform.h"

static enum obs_platform_type obs_platform = OBS_PLATFORM_DEFAULT;

static void *obs_platform_display = NULL;

void obs_set_platform(enum obs_platform_type platform)
{
	obs_platform = platform;
}

enum obs_platform_type obs_get_platform(void)
{
	return obs_platform;
}

void obs_set_platform_display(void *display)
{
	obs_platform_display = display;
}

void *obs_get_platform_display(void)
{
	return obs_platform_display;
}
