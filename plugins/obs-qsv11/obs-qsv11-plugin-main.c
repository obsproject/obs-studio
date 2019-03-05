/******************************************************************************
Copyright (C) 2013-2014 Intel Corporation
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
******************************************************************************/
#include <obs-module.h>
#include "mfxsession.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-qsv11", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Intel Quick Sync Video H.264 encoder support for Windows";
}

extern struct obs_encoder_info obs_qsv_encoder;

bool obs_module_load(void)
{
	mfxIMPL impl = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D11;
	mfxVersion ver = {{0 , 1}};
	mfxSession session;
	mfxStatus sts;

	sts = MFXInit(impl, &ver, &session);

	if (sts == MFX_ERR_NONE) {
		obs_register_encoder(&obs_qsv_encoder);
		MFXClose(session);
	} else {
		impl = MFX_IMPL_HARDWARE_ANY | MFX_IMPL_VIA_D3D9;
		sts = MFXInit(impl, &ver, &session);
		if (sts == MFX_ERR_NONE) {
			obs_register_encoder(&obs_qsv_encoder);
			MFXClose(session);
		}
	}

	return true;
}
