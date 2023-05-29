// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <obs-module.h>

#include "services-manager.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-services", "en-US")

bool obs_module_load(void)
{
	return ServicesManager::Initialize();
}

void obs_module_unload(void)
{
	ServicesManager::Finalize();
}
