/******************************************************************************
    Copyright (C) 2023-2024 by OBS Project

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

#include "vulkan-subsystem.h"
#include <util/blogging.h>
#include <util/mem.h>

/* Z-Stencil Buffer Functions */

/* These are primarily implemented in vulkan-stagesurf.c
   This file contains additional z-stencil specific functions if needed */

void gs_zstencilbuffer_zstencil_destroy(gs_zstencil_t *zs);
