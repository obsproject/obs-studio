#include "metal-subsystem.hpp"

void gs_device::RebuildDevice()
try {
	id<MTLDevice> dev;

	blog(LOG_WARNING, "Device Remove/Reset!  Rebuilding all assets...");

	/* ----------------------------------------------------------------- */

	gs_obj *obj = first_obj;

	while (obj) {
		switch (obj->obj_type) {
		case gs_type::gs_vertex_buffer:
			((gs_vertex_buffer*)obj)->Release();
			break;
		case gs_type::gs_index_buffer:
			((gs_index_buffer*)obj)->Release();
			break;
		case gs_type::gs_texture_2d:
			((gs_texture_2d*)obj)->Release();
			break;
		case gs_type::gs_zstencil_buffer:
			((gs_zstencil_buffer*)obj)->Release();
			break;
		case gs_type::gs_stage_surface:
			((gs_stage_surface*)obj)->Release();
			break;
		case gs_type::gs_sampler_state:
			((gs_sampler_state*)obj)->Release();
			break;
		case gs_type::gs_vertex_shader:
			((gs_vertex_shader*)obj)->Release();
			break;
		case gs_type::gs_pixel_shader:
			((gs_pixel_shader*)obj)->Release();
			break;
		case gs_type::gs_swap_chain:
			((gs_swap_chain*)obj)->Release();
			break;
		}

		obj = obj->next;
	}

	depthStencilState = nil;
	pipelineState = nil;
	commandBuffer = nil;
	commandQueue = nil;

	/* ----------------------------------------------------------------- */

	InitDevice(devIdx);

	dev = device;

	obj = first_obj;
	while (obj) {
		switch (obj->obj_type) {
		case gs_type::gs_vertex_buffer:
			((gs_vertex_buffer*)obj)->Rebuild();
			break;
		case gs_type::gs_index_buffer:
			((gs_index_buffer*)obj)->Rebuild();
			break;
		case gs_type::gs_texture_2d:
			((gs_texture_2d*)obj)->Rebuild();
			break;
		case gs_type::gs_zstencil_buffer:
			((gs_zstencil_buffer*)obj)->Rebuild();
			break;
		case gs_type::gs_stage_surface:
			((gs_stage_surface*)obj)->Rebuild();
			break;
		case gs_type::gs_sampler_state:
			((gs_sampler_state*)obj)->Rebuild();
			break;
		case gs_type::gs_vertex_shader:
			((gs_vertex_shader*)obj)->Rebuild();
			break;
		case gs_type::gs_pixel_shader:
			((gs_pixel_shader*)obj)->Rebuild();
			break;
		case gs_type::gs_swap_chain:
			((gs_swap_chain*)obj)->Rebuild();
			break;
		}

		obj = obj->next;
	}

	curRenderTarget = nullptr;
	curRenderSide = 0;
	curZStencilBuffer = nullptr;
	memset(&curTextures, 0, sizeof(curTextures));
	memset(&curSamplers, 0, sizeof(curSamplers));
	curVertexBuffer = nullptr;
	curIndexBuffer = nullptr;
	curVertexShader = nullptr;
	curPixelShader = nullptr;
	curSwapChain = nullptr;
	curStageSurface = nullptr;

	lastVertexBuffer = nullptr;
	lastVertexShader = nullptr;

	preserveClearTarget = nullptr;
	while (clearStates.size())
		clearStates.pop();

	while (projStack.size())
		projStack.pop();

} catch (const char *error) {
	bcrash("Failed to recreate Metal: %s", error);

}
