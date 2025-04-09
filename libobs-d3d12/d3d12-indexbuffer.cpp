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

void gs_index_buffer::Release()
{
	device->d3d12Instance->GetCommandManager().IdleGPU();
	if (indexBuffer) {
		delete indexBuffer;
		indexBuffer = nullptr;
	}
}

gs_index_buffer::~gs_index_buffer()
{
	Release();
}
