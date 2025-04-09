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

#include "d3d12-subsystem.hpp"

void gs_index_buffer::InitBuffer()
{
	indexBuffer = new D3D12Graphics::ByteAddressBuffer(device->d3d12Instance);
	indexBuffer->Create(L"Index Buffer", (uint32_t)num, (uint32_t)indexSize, indices.data);

	if (indexBuffer->GetResource() == nullptr) {
		throw HRError("Failed to create buffer", -1);
	}
}

gs_index_buffer::gs_index_buffer(gs_device_t *device, enum gs_index_type type, void *indices, size_t num,
				 uint32_t flags)
	: gs_obj(device, gs_type::gs_index_buffer),
	  dynamic((flags & GS_DYNAMIC) != 0),
	  type(type),
	  num(num),
	  indices(indices)
{
	switch (type) {
	case GS_UNSIGNED_SHORT:
		indexSize = 2;
		break;
	case GS_UNSIGNED_LONG:
		indexSize = 4;
		break;
	}

	InitBuffer();
}
