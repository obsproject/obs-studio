/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <util/base.h>
#include <graphics/vec3.h>
#include "d3d12-util.h"

class GSD3D12Device;

class GSD3D12VertBuffer {
public:
	GSD3D12VertBuffer(GSD3D12Device* device);
	bool Initialize(struct gs_vb_data *data, uint32_t flags);
	~GSD3D12VertBuffer();

private:
	GSD3D12Device *mGSD3D12Device = nullptr;
	ID3D12Resource *mVertexBuffer = nullptr;
};
