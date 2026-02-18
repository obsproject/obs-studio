#include <util/base.h>
#include <graphics/vec3.h>
#include "d3d12-subsystem.hpp"

static inline void PushBuffer(UINT *refNumBuffers, D3D12Graphics::GpuBuffer **buffers, uint32_t *strides,
			      D3D12Graphics::GpuBuffer *buffer, size_t elementSize, const char *name)
{
	const UINT numBuffers = *refNumBuffers;
	if (buffer) {
		buffers[numBuffers] = buffer;
		strides[numBuffers] = (uint32_t)elementSize;
		*refNumBuffers = numBuffers + 1;
	} else {
		blog(LOG_ERROR, "This vertex shader requires a %s buffer", name);
	}
}

void gs_vertex_buffer::FlushBuffer(D3D12Graphics::GpuBuffer *buffer, void *array, size_t elementSize)
{
	device->curContext->WriteBuffer(*buffer, 0, array, elementSize * vbd.data->num);
}

UINT gs_vertex_buffer::MakeBufferList(gs_vertex_shader *shader, D3D12Graphics::GpuBuffer **buffers, uint32_t *strides)
{
	UINT numBuffers = 0;
	PushBuffer(&numBuffers, buffers, strides, vertexBuffer, sizeof(vec3), "point");

	if (shader->hasNormals)
		PushBuffer(&numBuffers, buffers, strides, normalBuffer, sizeof(vec3), "normal");
	if (shader->hasColors)
		PushBuffer(&numBuffers, buffers, strides, colorBuffer, sizeof(uint32_t), "color");
	if (shader->hasTangents)
		PushBuffer(&numBuffers, buffers, strides, tangentBuffer, sizeof(vec3), "tangent");
	if (shader->nTexUnits <= uvBuffers.size()) {
		for (size_t i = 0; i < shader->nTexUnits; i++) {
			buffers[numBuffers] = uvBuffers[i];
			strides[numBuffers] = (uint32_t)uvSizes[i];
			++numBuffers;
		}
	} else {
		blog(LOG_ERROR,
		     "This vertex shader requires at least %u "
		     "texture buffers.",
		     (uint32_t)shader->nTexUnits);
	}

	return numBuffers;
}

void gs_vertex_buffer::InitBuffer(const size_t elementSize, const size_t numVerts, void *array,
				  D3D12Graphics::GpuBuffer **buffer)
{
	D3D12Graphics::GpuBuffer *byteBuffer = new D3D12Graphics::ByteAddressBuffer(device->d3d12Instance);
	byteBuffer->Create(L"Vertex Buffer", (uint32_t)numVerts, (uint32_t)elementSize, array);

	*buffer = byteBuffer;
	if (byteBuffer->GetResource() == nullptr) {
		throw HRError("Failed to create buffer", -1);
	}
}

void gs_vertex_buffer::BuildBuffers()
{
	InitBuffer(sizeof(vec3), vbd.data->num, vbd.data->points, &vertexBuffer);

	if (vbd.data->normals)
		InitBuffer(sizeof(vec3), vbd.data->num, vbd.data->normals, &normalBuffer);

	if (vbd.data->tangents)
		InitBuffer(sizeof(vec3), vbd.data->num, vbd.data->tangents, &tangentBuffer);

	if (vbd.data->colors)
		InitBuffer(sizeof(uint32_t), vbd.data->num, vbd.data->colors, &colorBuffer);

	for (size_t i = 0; i < vbd.data->num_tex; i++) {
		struct gs_tvertarray *tverts = vbd.data->tvarray + i;

		if (tverts->width != 2 && tverts->width != 4)
			throw "Invalid texture vertex size specified";
		if (!tverts->array)
			throw "No texture vertices specified";

		D3D12Graphics::GpuBuffer *buffer;
		InitBuffer(tverts->width * sizeof(float), vbd.data->num, tverts->array, &buffer);

		uvBuffers.push_back(buffer);
		uvSizes.push_back(tverts->width * sizeof(float));
	}
}

gs_vertex_buffer::~gs_vertex_buffer()
{
	Release();
}

void gs_vertex_buffer::Release()
{
	device->d3d12Instance->GetCommandManager().IdleGPU();
	if (vertexBuffer) {
		delete vertexBuffer;
		vertexBuffer = nullptr;
	}
	if (normalBuffer) {
		delete normalBuffer;
		normalBuffer = nullptr;
	}
	if (colorBuffer) {
		delete colorBuffer;
		colorBuffer = nullptr;
	}
	if (tangentBuffer) {
		delete tangentBuffer;
		tangentBuffer = nullptr;
	}

	for (auto buf : uvBuffers) {
		if (buf) {
			delete buf;
		}
	}

	uvBuffers.clear();
}

gs_vertex_buffer::gs_vertex_buffer(gs_device_t *device, struct gs_vb_data *data, uint32_t flags)
	: gs_obj(device, gs_type::gs_vertex_buffer),
	  dynamic((flags & GS_DYNAMIC) != 0),
	  vbd(data),
	  numVerts(data->num)
{
	if (!data->num)
		throw "Cannot initialize vertex buffer with 0 vertices";
	if (!data->points)
		throw "No points specified for vertex buffer";

	BuildBuffers();
}
