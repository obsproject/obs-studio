/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <QtWidgets/QMainWindow>
extern "C" {
#else
#include <stdint.h>
typedef struct QMainWindow QMainWindow;
#endif

void carla_qt_callback_on_main_thread(void (*callback)(void *param),
				      void *param);

const char *carla_qt_file_dialog(bool save, bool isDir, const char *title,
				 const char *filter);

#ifdef __cplusplus
}
#endif
