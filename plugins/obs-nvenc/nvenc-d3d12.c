#include "nvenc-internal.h"
#include "nvenc-helpers.h"

static HANDLE get_lib(struct nvenc_data *enc, const char *lib)
{
	HMODULE mod = GetModuleHandleA(lib);
	if (mod)
		return mod;

	mod = LoadLibraryA(lib);
	if (!mod)
		error("Failed to load %s", lib);
	return mod;
}

typedef HRESULT(WINAPI *CREATEDXGIFACTORY2PROC)(UINT, REFIID, void **);

bool d3d12_init(struct nvenc_data *enc, obs_data_t *settings)
{
	HMODULE dxgi = get_lib(enc, "DXGI.dll");
	HMODULE d3d12 = get_lib(enc, "D3D12.dll");
	CREATEDXGIFACTORY2PROC create_dxgi;
	PFN_D3D12_CREATE_DEVICE create_device;
	IDXGIFactory6 *factory;
	IDXGIAdapter1 *adapter1;
	ID3D12Device *device;
	ID3D12Fence *fence;
	ID3D12CommandQueue *command_queue;
	ID3D12GraphicsCommandList *command_list = NULL;
	ID3D12CommandAllocator *allocator = NULL;
	HANDLE event;

	ID3D12Fence *input_fence;
	ID3D12Fence *output_fence;

	HRESULT hr;

	if (!dxgi || !d3d12) {
		return false;
	}

	create_dxgi = (CREATEDXGIFACTORY2PROC)GetProcAddress(dxgi, "CreateDXGIFactory2");
	create_device = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12, "D3D12CreateDevice");
#if 0
	bool create_debug = 0;
	ID3D12Debug *debug_interface;
	if (create_debug) {
		PFN_D3D12_GET_DEBUG_INTERFACE get_debug_interface_func;
		get_debug_interface_func =
			(PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12, "D3D12GetDebugInterface");
		if (SUCCEEDED(get_debug_interface_func(&IID_ID3D12Debug, (void **)&debug_interface))) {
			debug_interface->lpVtbl->EnableDebugLayer(debug_interface);

			uint32_t useGPUBasedValidation = 1;
			if (useGPUBasedValidation) {
				ID3D12Debug1* debugInterface1;
				if (SUCCEEDED((debug_interface->lpVtbl->QueryInterface(
					    debug_interface, &IID_ID3D12Debug1, &debugInterface1)))) {
					debugInterface1->lpVtbl->SetEnableGPUBasedValidation(debugInterface1, true);
				}
			}
		}

		IDXGIInfoQueue *dxgi_Info_queue;
		CREATEDXGIFACTORY2PROC dxgi_get_debug_interface1;
		dxgi_get_debug_interface1 = (CREATEDXGIFACTORY2PROC)GetProcAddress(dxgi, "DXGIGetDebugInterface1");
		HRESULT result = dxgi_get_debug_interface1(0, &IID_IDXGIInfoQueue, (void **)&dxgi_Info_queue);
		if (FAILED(result)) {
		}

		result = dxgi_Info_queue->lpVtbl->SetBreakOnSeverity(dxgi_Info_queue, DXGI_DEBUG_ALL,
						  DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
		result = dxgi_Info_queue->lpVtbl->SetBreakOnSeverity(dxgi_Info_queue, DXGI_DEBUG_ALL,
						  DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);

		DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
			80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */
			,
		};
		DXGI_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		dxgi_Info_queue->lpVtbl->AddStorageFilterEntries(dxgi_Info_queue, DXGI_DEBUG_DXGI, &filter);
	}
#endif

	if (!create_dxgi || !create_device) {
		error("Failed to load D3D12/DXGI procedures");
		return false;
	}

	hr = create_dxgi(/* create_debug ? DXGI_CREATE_FACTORY_DEBUG :*/ 0, &IID_IDXGIFactory6, &factory);
	if (FAILED(hr)) {
		error_hr("CreateDXGIFactory1 failed");
		return false;
	}

	hr = factory->lpVtbl->EnumAdapterByGpuPreference(factory, 0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
							 &IID_IDXGIAdapter1, &adapter1);
	factory->lpVtbl->Release(factory);
	if (FAILED(hr)) {
		error_hr("EnumAdapters failed");
		return false;
	}

	hr = create_device((IUnknown *)adapter1, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, (void **)&device);
	adapter1->lpVtbl->Release(adapter1);
	if (FAILED(hr)) {
		error_hr("D3D12CreateDevice failed");
		return false;
	}
#if 0
	if (create_debug) {
		ID3D12InfoQueue *pInfoQueue;
		if (SUCCEEDED(device->lpVtbl->QueryInterface(device, &IID_ID3D12InfoQueue, &pInfoQueue))) {
			D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
			D3D12_MESSAGE_ID DenyIds[] = {
				D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
				D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,
				D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS,
				D3D12_MESSAGE_ID_RESOLVE_QUERY_INVALID_QUERY_STATE,
				D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
			};

			D3D12_INFO_QUEUE_FILTER NewFilter;
			memset(&NewFilter, 0, sizeof(D3D12_INFO_QUEUE_FILTER));
			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;
			NewFilter.DenyList.NumIDs = _countof(DenyIds);
			NewFilter.DenyList.pIDList = DenyIds;

			pInfoQueue->lpVtbl->PushStorageFilter(pInfoQueue, &NewFilter);
			pInfoQueue->lpVtbl->Release(pInfoQueue);
		}
	}
#endif
	hr = device->lpVtbl->CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)(&fence));
	if (FAILED(hr)) {
		error_hr("D3D12 CreateFence failed");
		return false;
	}

	hr = device->lpVtbl->CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)(&input_fence));
	if (FAILED(hr)) {
		error_hr("D3D12 CreateFence for input fence failed");
		return false;
	}

	hr = device->lpVtbl->CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)(&output_fence));
	if (FAILED(hr)) {
		error_hr("D3D12 CreateFence for output fence failed");
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC QueueDesc;
	memset(&QueueDesc, 0, sizeof(D3D12_COMMAND_QUEUE_DESC));
	QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	QueueDesc.NodeMask = 1;
	hr = device->lpVtbl->CreateCommandQueue(device, &QueueDesc, &IID_ID3D12CommandQueue, (void **)(&command_queue));
	if (FAILED(hr)) {
		error_hr("D3D12 CreateCommandQueue failed");
		return false;
	}

	for (size_t i = 0; i < 64; ++i) {
		hr = device->lpVtbl->CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_COPY,
							    &IID_ID3D12CommandAllocator, (void **)(&allocator));
		if (FAILED(hr)) {
			error_hr("D3D12 CreateCommandAllocator failed");
			return false;
		}
		enc->allocators[i] = allocator;
	}

	if (enc->allocators[0] != NULL) {
		hr = device->lpVtbl->CreateCommandList(device, 1, D3D12_COMMAND_LIST_TYPE_COPY, enc->allocators[0],
						       NULL, &IID_ID3D12GraphicsCommandList, (void **)(&command_list));
		if (FAILED(hr)) {
			error_hr("D3D12 CreateCommandList failed");
			return false;
		}

		command_list->lpVtbl->Close(command_list);
	}

	event = CreateEvent(NULL, false, false, NULL);

	enc->device12 = device;

	enc->command_queue = command_queue;
	enc->event = event;
	enc->command_list = command_list;

	enc->input_fence = input_fence;
	enc->input_fence_value = 0;

	enc->output_fence = output_fence;
	enc->output_fence_value = 0;
	return true;
}

void d3d12_free(struct nvenc_data *enc)
{
	for (size_t i = 0; i < enc->input_textures.num; i++) {
		ID3D12Resource *tex = enc->input_textures.array[i].tex;
		tex->lpVtbl->Release(tex);
	}

	if (enc->command_queue) {
		enc->command_queue->lpVtbl->Release(enc->command_queue);
	}

	if (enc->command_list) {
		enc->command_list->lpVtbl->Release(enc->command_list);
	}

	if (enc->input_fence) {
		enc->input_fence->lpVtbl->Release(enc->input_fence);
	}

	if (enc->output_fence) {
		enc->output_fence->lpVtbl->Release(enc->output_fence);
	}

	if (enc->event) {
		CloseHandle(enc->event);
	}

	for (size_t i = 0; i < _countof(enc->allocators); ++i) {
		if (enc->allocators[i]) {
			enc->allocators[i]->lpVtbl->Release(enc->allocators[i]);
		}
	}

	if (enc->device12) {
		enc->device12->lpVtbl->Release(enc->device12);
	}
}

static bool d3d12_texture_init(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	ID3D12Device *const device = enc->device12;
	const bool p010 = obs_encoder_video_tex_active(enc->encoder, VIDEO_FORMAT_P010);

	D3D12_RESOURCE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = enc->cx;
	desc.Height = enc->cy;
	desc.MipLevels = 1;
	desc.DepthOrArraySize = 1;
	desc.Format = p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12;
	desc.SampleDesc.Count = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	ID3D12Resource *tex;
	HRESULT hr = device->lpVtbl->CreateCommittedResource(device, &HeapProps, D3D12_HEAP_FLAG_NONE, &desc,
							     D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource,
							     &tex);
	if (FAILED(hr)) {
		error_hr("Failed to create texture");
		return false;
	}

	NV_ENC_FENCE_POINT_D3D12 regRsrcInputFence;
	// Set input fence point
	memset(&regRsrcInputFence, 0, sizeof(NV_ENC_FENCE_POINT_D3D12));
	regRsrcInputFence.pFence = enc->input_fence;
	regRsrcInputFence.waitValue = enc->input_fence_value;
	regRsrcInputFence.bWait = true;

	enc->input_fence_value++;

	regRsrcInputFence.signalValue = enc->input_fence_value;
	regRsrcInputFence.bSignal = true;

	NV_ENC_REGISTER_RESOURCE res = {NV_ENC_REGISTER_RESOURCE_VER};
	res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	res.resourceToRegister = tex;
	res.width = enc->cx;
	res.height = enc->cy;
	res.bufferFormat = p010 ? NV_ENC_BUFFER_FORMAT_YUV420_10BIT : NV_ENC_BUFFER_FORMAT_NV12;
	res.bufferUsage = NV_ENC_INPUT_IMAGE;
	res.pInputFencePoint = &regRsrcInputFence;
	if (NV_FAILED(nv.nvEncRegisterResource(enc->session, &res))) {
		tex->lpVtbl->Release(tex);
		return false;
	}

	NV_ENC_INPUT_RESOURCE_D3D12 *input_res =
		(NV_ENC_INPUT_RESOURCE_D3D12 *)bmalloc(sizeof(NV_ENC_INPUT_RESOURCE_D3D12));
	memset(input_res, 0, sizeof(NV_ENC_INPUT_RESOURCE_D3D12));
	input_res->inputFencePoint.pFence = enc->input_fence;

	nvtex->res = res.registeredResource;
	nvtex->tex = tex;
	nvtex->mapped_res = NULL;
	nvtex->input_res = input_res;

	cpu_wait_for_fence_point(enc, regRsrcInputFence.pFence, regRsrcInputFence.signalValue);

	return true;
}

bool d3d12_init_textures(struct nvenc_data *enc)
{
	da_reserve(enc->textures, enc->buf_count);
	for (uint32_t i = 0; i < enc->buf_count; i++) {
		struct nv_texture texture;
		if (!d3d12_texture_init(enc, &texture)) {
			return false;
		}

		da_push_back(enc->textures, &texture);
	}

	return true;
}

static void d3d12_texture_free(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	if (nvtex->res) {
		if (nvtex->mapped_res) {
			nv.nvEncUnmapInputResource(enc->session, nvtex->mapped_res);
		}
		nv.nvEncUnregisterResource(enc->session, nvtex->res);
		ID3D12Resource *tex12 = (ID3D12Resource *)(nvtex->tex);
		tex12->lpVtbl->Release(tex12);
		bfree(nvtex->input_res);
	}
}

void d3d12_free_textures(struct nvenc_data *enc)
{
	for (size_t i = 0; i < enc->textures.num; i++) {
		d3d12_texture_free(enc, &enc->textures.array[i]);
	}
}

void cpu_wait_for_fence_point(struct nvenc_data *enc, ID3D12Fence *fence, uint64_t fence_value)
{
	if (fence->lpVtbl->GetCompletedValue(fence) < fence_value) {
		if (fence->lpVtbl->SetEventOnCompletion(fence, fence_value, enc->event) != S_OK) {
			return;
		}
		WaitForSingleObject(enc->event, INFINITE);
	}
}

bool d3d12_init_readback(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	ID3D12Device *const device = enc->device12;
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC ResourceDesc;
	memset(&ResourceDesc, 0, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = 2 * enc->cx * enc->cy;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource *tex = NULL;
	HRESULT hr = device->lpVtbl->CreateCommittedResource(device, &HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
							     D3D12_RESOURCE_STATE_COPY_DEST, NULL, &IID_ID3D12Resource,
							     (void **)(&tex));
	if (FAILED(hr)) {
		auto remoteReason = device->lpVtbl->GetDeviceRemovedReason(device);
		error_hr("Failed to create texture");
		return false;
	}

	NV_ENC_REGISTER_RESOURCE res = {NV_ENC_REGISTER_RESOURCE_VER};
	res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	res.resourceToRegister = tex;
	res.width = enc->cx;
	res.height = enc->cy;
	res.bufferFormat = NV_ENC_BUFFER_FORMAT_U8;
	res.bufferUsage = NV_ENC_OUTPUT_BITSTREAM;
	res.pInputFencePoint = NULL;
	if (NV_FAILED(nv.nvEncRegisterResource(enc->session, &res))) {
		tex->lpVtbl->Release(tex);
		return false;
	}

	struct nv_output *output = (struct nv_output *)bmalloc(sizeof(struct nv_output));

	NV_ENC_OUTPUT_RESOURCE_D3D12 *output_res =
		(NV_ENC_OUTPUT_RESOURCE_D3D12 *)bmalloc(sizeof(NV_ENC_OUTPUT_RESOURCE_D3D12));
	memset(output_res, 0, sizeof(NV_ENC_OUTPUT_RESOURCE_D3D12));
	output_res->outputFencePoint.pFence = enc->output_fence;

	output->res = res.registeredResource;
	output->tex = tex;
	output->mapped_res = NULL;
	output->output_res = output_res;

	bs->ptr = output;
	return true;
}

void d3d12_free_readback(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	if (bs->ptr) {
		struct nv_output *output = (struct nv_output *)bs->ptr;
		if (output->mapped_res) {
			nv.nvEncUnmapInputResource(enc->session, output->mapped_res);
		}
		nv.nvEncUnregisterResource(enc->session, output->res);
		ID3D12Resource *tex12 = (ID3D12Resource *)(output->tex);
		tex12->lpVtbl->Release(tex12);
		bfree(output->output_res);
		bfree(output);
	}
}

static ID3D12Resource *get_tex_from_handle(struct nvenc_data *enc, uint32_t handle, IDXGIKeyedMutex **km_out)
{
	ID3D12Device *device = enc->device12;
	ID3D12Resource *input_tex;
	HRESULT hr;

	for (size_t i = 0; i < enc->input_textures.num; i++) {
		struct handle_tex *ht = &enc->input_textures.array[i];
		if (ht->handle == handle) {
			*km_out = ht->km;
			return ht->tex;
		}
	}

	hr = device->lpVtbl->OpenSharedHandle(device, (HANDLE)(uintptr_t)handle, &IID_ID3D12Resource, &input_tex);
	if (FAILED(hr)) {
		error_hr("OpenSharedResource failed");
		return NULL;
	}

	*km_out = NULL;
	struct handle_tex new_ht = {handle, input_tex, NULL};
	da_push_back(enc->input_textures, &new_ht);
	return input_tex;
}

bool d3d12_encode(void *data, struct encoder_texture *texture, int64_t pts, uint64_t lock_key, uint64_t *next_key,
		  struct encoder_packet *packet, bool *received_packet)
{
	struct nvenc_data *enc = data;
	ID3D12Resource *input_tex;
	ID3D12Resource *output_tex;
	IDXGIKeyedMutex *km;
	struct nv_texture *nvtex;
	struct nv_bitstream *bs;

	ID3D12GraphicsCommandList *command_list = enc->command_list;
	ID3D12CommandQueue *command_queue = enc->command_queue;
	ID3D12Fence *input_fence = enc->input_fence;

	if (texture->handle == GS_INVALID_HANDLE) {
		error("Encode failed: bad texture handle");
		*next_key = lock_key;
		return false;
	}

	bs = &enc->bitstreams.array[enc->next_bitstream];
	nvtex = &enc->textures.array[enc->next_bitstream];

	input_tex = get_tex_from_handle(enc, texture->handle, &km);
	output_tex = nvtex->tex;

	if (!input_tex) {
		*next_key = lock_key;
		return false;
	}

	deque_push_back(&enc->dts_list, &pts, sizeof(pts));
	ID3D12CommandAllocator *allocator = enc->allocators[enc->next_bitstream];
	HRESULT hr = allocator->lpVtbl->Reset(allocator);
	if (FAILED(hr)) {
		error_hr("CommandAllocator(Reset) failed");
		return false;
	}

	hr = command_list->lpVtbl->Reset(command_list, allocator, NULL);
	if (FAILED(hr)) {
		error_hr("CommandList(Reset) failed");
		return false;
	}

	command_list->lpVtbl->CopyResource(command_list, output_tex, input_tex);

	hr = command_list->lpVtbl->Close(command_list);
	if (FAILED(hr)) {
		error_hr("CommandList(Close) failed");
		return false;
	}

	command_queue->lpVtbl->ExecuteCommandLists(command_queue, 1, (ID3D12CommandList **)&command_list);
	++enc->input_fence_value;
	hr = command_queue->lpVtbl->Signal(command_queue, input_fence, enc->input_fence_value);
	if (FAILED(hr)) {
		error_hr("CommandQeue(Signal) failed");
		return false;
	}

	NV_ENC_MAP_INPUT_RESOURCE mapIn = {NV_ENC_MAP_INPUT_RESOURCE_VER};
	mapIn.registeredResource = nvtex->res;
	if (NV_FAILED(nv.nvEncMapInputResource(enc->session, &mapIn))) {
		return false;
	}
	nvtex->mapped_res = mapIn.mappedResource;

	NV_ENC_INPUT_RESOURCE_D3D12 *input_res = nvtex->input_res;
	input_res->version = NV_ENC_INPUT_RESOURCE_D3D12_VER;
	input_res->pInputBuffer = nvtex->mapped_res;
	input_res->inputFencePoint.waitValue = enc->input_fence_value;
	input_res->inputFencePoint.bWait = true;

	NV_ENC_MAP_INPUT_RESOURCE mapOut = {NV_ENC_MAP_INPUT_RESOURCE_VER};
	struct nv_output *output = (struct nv_output *)bs->ptr;

	mapOut.registeredResource = output->res;
	if (NV_FAILED(nv.nvEncMapInputResource(enc->session, &mapOut))) {
		return false;
	}

	output->mapped_res = mapOut.mappedResource;

	// output
	enc->output_fence_value++;
	NV_ENC_OUTPUT_RESOURCE_D3D12 *output_res = output->output_res;
	output_res->version = NV_ENC_OUTPUT_RESOURCE_D3D12_VER;
	output_res->pOutputBuffer = output->mapped_res;
	output_res->outputFencePoint.signalValue = enc->output_fence_value;
	output_res->outputFencePoint.bSignal = true;

	return nvenc_encode_base(enc, bs, nvtex->input_res, pts, packet, received_packet);
}
