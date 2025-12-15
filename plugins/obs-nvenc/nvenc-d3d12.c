#include "nvenc-internal.h"
#include "nvenc-helpers.h"

/*
 * NVENC implementation using Direct3D 11 context and textures
 */

/* ------------------------------------------------------------------------- */
/* D3D11 Context/Device management                                           */

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

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID, void **);

bool d3d12_init(struct nvenc_data *enc, obs_data_t *settings)
{
	HMODULE dxgi = get_lib(enc, "DXGI.dll");
	HMODULE d3d12 = get_lib(enc, "D3D12.dll");
	CREATEDXGIFACTORY1PROC create_dxgi;
	PFN_D3D12_CREATE_DEVICE create_device;
	IDXGIFactory1 *factory;
	IDXGIAdapter *adapter;
	ID3D12Device *device;
	ID3D12Fence *fence;
	HRESULT hr;

	if (!dxgi || !d3d12) {
		return false;
	}

	create_dxgi = (CREATEDXGIFACTORY1PROC)GetProcAddress(dxgi, "CreateDXGIFactory1");
	create_device = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12, "D3D12CreateDevice");

	if (!create_dxgi || !create_device) {
		error("Failed to load D3D12/DXGI procedures");
		return false;
	}

	hr = create_dxgi(&IID_IDXGIFactory1, &factory);
	if (FAILED(hr)) {
		error_hr("CreateDXGIFactory1 failed");
		return false;
	}

	hr = factory->lpVtbl->EnumAdapters(factory, 0, &adapter);
	factory->lpVtbl->Release(factory);
	if (FAILED(hr)) {
		error_hr("EnumAdapters failed");
		return false;
	}

	hr = create_device((IUnknown*)adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, (void**)&device);
	adapter->lpVtbl->Release(adapter);
	if (FAILED(hr)) {
		error_hr("D3D12CreateDevice failed");
		return false;
	}

	hr = device->lpVtbl->CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void**)(&fence));
	if (FAILED(hr)) {
		error_hr("D3D12 CreateFence failed");
		return false;
	}

	enc->device12 = device;
	enc->fence = fence;
	return true;
}

void d3d12_free(struct nvenc_data *enc)
{
	for (size_t i = 0; i < enc->input_textures.num; i++) {
		ID3D11Texture2D *tex = enc->input_textures.array[i].tex;
		IDXGIKeyedMutex *km = enc->input_textures.array[i].km;
		tex->lpVtbl->Release(tex);
		km->lpVtbl->Release(km);
	}

	if (enc->device12) {
		enc->device12->lpVtbl->Release(enc->device12);
	}
}

/* ------------------------------------------------------------------------- */
/* D3D11 Surface management                                                  */

static bool d3d12_texture_init(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	const bool p010 = obs_encoder_video_tex_active(enc->encoder, VIDEO_FORMAT_P010);

	D3D12_RESOURCE_DESC desc = {0};
	desc.Width = enc->cx;
	desc.Height = enc->cy;
	desc.MipLevels = 1;
	desc.DepthOrArraySize = 1;
	desc.Format = p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12;
	desc.SampleDesc.Count = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ID3D12Device *const device = enc->device12;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	ID3D12Resource *tex;
	HRESULT hr = device->lpVtbl->CreateCommittedResource(device, &HeapProps, D3D12_HEAP_FLAG_NONE, &desc,
							     D3D12_RESOURCE_STATE_COMMON,  NULL, &IID_ID3D12Resource, &tex);
	if (FAILED(hr)) {
		error_hr("Failed to create texture");
		return false;
	}

	NV_ENC_REGISTER_RESOURCE res = {NV_ENC_REGISTER_RESOURCE_VER};
	res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	res.resourceToRegister = tex;
	res.width = enc->cx;
	res.height = enc->cy;
	res.bufferFormat = p010 ? NV_ENC_BUFFER_FORMAT_YUV420_10BIT : NV_ENC_BUFFER_FORMAT_NV12;

	NV_ENC_FENCE_POINT_D3D12 d3d12_enc_fence = {NV_ENC_FENCE_POINT_D3D12_VER};
	d3d12_enc_fence.bSignal = 0;
	d3d12_enc_fence.pFence = enc->fence;
	d3d12_enc_fence.bWait = 0;
	d3d12_enc_fence.signalValue = 0;
	d3d12_enc_fence.waitValue = 0;

	res.pInputFencePoint = &d3d12_enc_fence;
	if (NV_FAILED(nv.nvEncRegisterResource(enc->session, &res))) {
		tex->lpVtbl->Release(tex);
		return false;
	}

	nvtex->res = res.registeredResource;
	nvtex->tex12 = tex;
	nvtex->mapped_res = NULL;
	return true;
}

bool d3d12_init_textures(struct nvenc_data *enc)
{
	//blog(LOG_DEBUG, "buf count: %d", enc->buf_count);
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
		nvtex->tex->lpVtbl->Release(nvtex->tex);
	}
}

void d3d12_free_textures(struct nvenc_data *enc)
{
	for (size_t i = 0; i < enc->textures.num; i++) {
		d3d12_texture_free(enc, &enc->textures.array[i]);
	}
}

/* ------------------------------------------------------------------------- */
/* Actual encoding stuff                                                     */

static ID3D11Texture2D *get_tex_from_handle(struct nvenc_data *enc, uint32_t handle, IDXGIKeyedMutex **km_out)
{
	ID3D12Device *device = enc->device12;
	// IDXGIKeyedMutex *km;
	// ID3D11Texture2D *input_tex;
	// HRESULT hr;

	for (size_t i = 0; i < enc->input_textures.num; i++) {
		struct handle_tex *ht = &enc->input_textures.array[i];
		if (ht->handle == handle) {
			*km_out = ht->km;
			return ht->tex;
		}
	}

	return NULL;
	/* hr =
		device->lpVtbl->OpenSharedResource(device, (HANDLE)(uintptr_t)handle, &IID_ID3D11Texture2D, &input_tex);
	if (FAILED(hr)) {
		error_hr("OpenSharedResource failed");
		return NULL;
	}

	hr = input_tex->lpVtbl->QueryInterface(input_tex, &IID_IDXGIKeyedMutex, &km);
	if (FAILED(hr)) {
		error_hr("QueryInterface(IDXGIKeyedMutex) failed");
		input_tex->lpVtbl->Release(input_tex);
		return NULL;
	}

	input_tex->lpVtbl->SetEvictionPriority(input_tex, DXGI_RESOURCE_PRIORITY_MAXIMUM);

	*km_out = km;

	struct handle_tex new_ht = {handle, input_tex, km};
	da_push_back(enc->input_textures, &new_ht);
	return input_tex;*/
}

bool d3d12_encode(void *data, struct encoder_texture *texture, int64_t pts, uint64_t lock_key, uint64_t *next_key,
		  struct encoder_packet *packet, bool *received_packet)
{
	struct nvenc_data *enc = data;
	ID3D11DeviceContext *context = enc->context;
	ID3D11Texture2D *input_tex;
	ID3D11Texture2D *output_tex;
	IDXGIKeyedMutex *km;
	struct nv_texture *nvtex;
	struct nv_bitstream *bs;

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

	/* ------------------------------------ */
	/* copy to output tex                   */

	km->lpVtbl->AcquireSync(km, lock_key, INFINITE);

	context->lpVtbl->CopyResource(context, (ID3D11Resource *)output_tex, (ID3D11Resource *)input_tex);

	km->lpVtbl->ReleaseSync(km, *next_key);

	/* ------------------------------------ */
	/* map output tex so nvenc can use it   */

	NV_ENC_MAP_INPUT_RESOURCE map = {NV_ENC_MAP_INPUT_RESOURCE_VER};
	map.registeredResource = nvtex->res;
	if (NV_FAILED(nv.nvEncMapInputResource(enc->session, &map))) {
		return false;
	}

	nvtex->mapped_res = map.mappedResource;

	/* ------------------------------------ */
	/* do actual encode call                */

	return nvenc_encode_base(enc, bs, nvtex->mapped_res, pts, packet, received_packet);
}
