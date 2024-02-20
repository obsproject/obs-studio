/*

This file is provided under a dual BSD/GPLv2 license.  When using or
redistributing this file, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) Oct. 2015 Intel Corporation.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Seung-Woo Kim, seung-woo.kim@intel.com
705 5th Ave S #500, Seattle, WA 98104

BSD LICENSE

Copyright(c) <date> Intel Corporation.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <obs-module.h>

#include "common_utils.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-qsv11", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Intel Quick Sync Video support for Windows";
}

extern struct obs_encoder_info obs_qsv_encoder;
extern struct obs_encoder_info obs_qsv_encoder_v2;
extern struct obs_encoder_info obs_qsv_encoder_tex;
extern struct obs_encoder_info obs_qsv_encoder_tex_v2;
extern struct obs_encoder_info obs_qsv_av1_encoder_tex;
extern struct obs_encoder_info obs_qsv_av1_encoder;
extern struct obs_encoder_info obs_qsv_hevc_encoder_tex;
extern struct obs_encoder_info obs_qsv_hevc_encoder;

bool obs_module_load(void)
{
	adapter_count = MAX_ADAPTERS;
	check_adapters(adapters, &adapter_count);

	bool avc_supported = false;
	bool av1_supported = false;
	bool hevc_supported = false;
	for (size_t i = 0; i < adapter_count; i++) {
		struct adapter_info *adapter = &adapters[i];
		avc_supported |= adapter->is_intel;
		av1_supported |= adapter->supports_av1;
		hevc_supported |= adapter->supports_hevc;
	}

	if (avc_supported) {
		obs_register_encoder(&obs_qsv_encoder_tex_v2);
		obs_register_encoder(&obs_qsv_encoder_tex);
		obs_register_encoder(&obs_qsv_encoder_v2);
		obs_register_encoder(&obs_qsv_encoder);
	}
	if (av1_supported) {
		obs_register_encoder(&obs_qsv_av1_encoder_tex);
		obs_register_encoder(&obs_qsv_av1_encoder);
	}
#if ENABLE_HEVC
	if (hevc_supported) {
		obs_register_encoder(&obs_qsv_hevc_encoder_tex);
		obs_register_encoder(&obs_qsv_hevc_encoder);
	}
#endif

	return true;
}
