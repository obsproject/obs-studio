#pragma once

#include <util/platform.h>

bool nvenc_supported(void);

void obs_nvenc_load(void);
void obs_nvenc_unload(void);

void obs_cuda_load(void);
void obs_cuda_unload(void);
