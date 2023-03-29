#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "../external/nvEncodeAPI.h"
#include "../jim-nvenc-ver.h"

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

__declspec(dllexport) DWORD NvOptimusEnablement = 1;
NV_ENCODE_API_FUNCTION_LIST nv = {NV_ENCODE_API_FUNCTION_LIST_VER};
static void *nvenc_lib = NULL;
static bool av1_supported = false;

#define NVIDIA_VENDOR_ID 0x10DE

struct nvenc_info {
	bool is_nvidia;
	bool supports_av1;
};

#define MAX_CAPS 10
static uint32_t luid_count = 0;
static uint64_t luid_order[MAX_CAPS] = {0};
static struct nvenc_info adapter_info[MAX_CAPS] = {0};

bool load_nvenc_lib(void)
{
	const char *const file = (sizeof(void *) == 8) ? "nvEncodeAPI64.dll"
						       : "nvEncodeAPI.dll";
	nvenc_lib = LoadLibraryA(file);
	return nvenc_lib != NULL;
}

static inline void *load_nv_func(const char *func)
{
	void *func_ptr = (void *)GetProcAddress(nvenc_lib, func);
	return func_ptr;
}

static inline uint32_t get_adapter_idx(uint32_t adapter_idx, LUID luid)
{
	for (uint32_t i = 0; i < luid_count; i++) {
		if (luid_order[i] == *(uint64_t *)&luid) {
			return i;
		}
	}

	return adapter_idx;
}

static bool get_adapter_caps(IDXGIFactory *factory, uint32_t adapter_idx)
{
	struct nvenc_info *caps;
	IDXGIAdapter *adapter = NULL;
	ID3D11Device *device = NULL;
	ID3D11DeviceContext *context = NULL;
	GUID *guids = NULL;
	void *session = NULL;
	HRESULT hr;

	if (adapter_idx == MAX_CAPS)
		return false;

	hr = factory->lpVtbl->EnumAdapters(factory, adapter_idx, &adapter);
	if (FAILED(hr))
		return false;

	DXGI_ADAPTER_DESC desc;
	adapter->lpVtbl->GetDesc(adapter, &desc);

	caps = &adapter_info[get_adapter_idx(adapter_idx, desc.AdapterLuid)];
	if (desc.VendorId != NVIDIA_VENDOR_ID)
		return true;

	caps->is_nvidia = true;

	hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL,
			       0, D3D11_SDK_VERSION, &device, NULL, &context);
	if (FAILED(hr))
		goto finish;

	/* ---------------------------------------------------------------- */

	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {
		NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER};
	params.device = device;
	params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	params.apiVersion = NVENCAPI_VERSION;

	NVENCSTATUS stat = nv.nvEncOpenEncodeSessionEx(&params, &session);
	if (stat != NV_ENC_SUCCESS)
		goto finish;

	uint32_t guid_count = 0;
	if (nv.nvEncGetEncodeGUIDCount(session, &guid_count) != NV_ENC_SUCCESS)
		goto finish;

	guids = malloc(guid_count * sizeof(GUID));
	stat = nv.nvEncGetEncodeGUIDs(session, guids, guid_count, &guid_count);
	if (stat != NV_ENC_SUCCESS)
		goto finish;

	for (uint32_t i = 0; i < guid_count; i++) {
		GUID *guid = &guids[i];

		if (memcmp(guid, &NV_ENC_CODEC_AV1_GUID, sizeof(GUID)) == 0) {
			caps->supports_av1 = true;
			break;
		}
	}

finish:
	if (guids)
		free(guids);
	if (session)
		nv.nvEncDestroyEncoder(session);
	if (context)
		context->lpVtbl->Release(context);
	if (device)
		device->lpVtbl->Release(device);
	if (adapter)
		adapter->lpVtbl->Release(adapter);
	return true;
}

typedef NVENCSTATUS(NVENCAPI *NV_MAX_VER_FUNC)(uint32_t *);
typedef NVENCSTATUS(NVENCAPI *NV_CREATE_INSTANCE_FUNC)(
	NV_ENCODE_API_FUNCTION_LIST *);

static inline uint32_t get_nvenc_ver(void)
{
	NV_MAX_VER_FUNC nv_max_ver = (NV_MAX_VER_FUNC)load_nv_func(
		"NvEncodeAPIGetMaxSupportedVersion");
	if (!nv_max_ver) {
		return 0;
	}

	uint32_t ver = 0;
	if (nv_max_ver(&ver) != NV_ENC_SUCCESS) {
		return 0;
	}
	return ver;
}

static inline bool init_nvenc_internal(void)
{
	if (!load_nvenc_lib())
		return false;

	uint32_t ver = get_nvenc_ver();
	if (ver == 0)
		return false;

	uint32_t supported_ver = (NVENC_COMPAT_MAJOR_VER << 4) |
				 NVENC_COMPAT_MINOR_VER;
	if (supported_ver > ver)
		return false;

	NV_CREATE_INSTANCE_FUNC nv_create_instance =
		(NV_CREATE_INSTANCE_FUNC)load_nv_func(
			"NvEncodeAPICreateInstance");
	if (!nv_create_instance)
		return false;

	return nv_create_instance(&nv) == NV_ENC_SUCCESS;
}

DWORD WINAPI TimeoutThread(LPVOID param)
{
	HANDLE hMainThread = (HANDLE)param;

	DWORD ret = WaitForSingleObject(hMainThread, 2500);
	if (ret == WAIT_TIMEOUT)
		TerminateProcess(GetCurrentProcess(), STATUS_TIMEOUT);

	CloseHandle(hMainThread);
	return 0;
}

int main(int argc, char *argv[])
{
	IDXGIFactory *factory = NULL;
	HRESULT hr;

	HANDLE hMainThread;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
			GetCurrentProcess(), &hMainThread, 0, FALSE,
			DUPLICATE_SAME_ACCESS);
	DWORD threadId;
	HANDLE hThread;
	hThread =
		CreateThread(NULL, 0, TimeoutThread, hMainThread, 0, &threadId);
	CloseHandle(hThread);

	/* --------------------------------------------------------- */
	/* try initializing nvenc, I guess                           */

	if (!init_nvenc_internal())
		return 0;

	/* --------------------------------------------------------- */
	/* parse expected LUID order                                 */

	luid_count = argc - 1;
	for (int i = 1; i < argc; i++) {
		luid_order[i - 1] = strtoull(argv[i], NULL, 16);
	}

	/* --------------------------------------------------------- */
	/* obtain adapter compatibility information                  */

	hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory);
	if (FAILED(hr))
		return 0;

	uint32_t idx = 0;
	while (get_adapter_caps(factory, idx++))
		;

	for (uint32_t i = 0; i < idx; i++) {
		struct nvenc_info caps = adapter_info[i];

		printf("[%u]\n", i);
		printf("is_nvidia=%s\n", caps.is_nvidia ? "true" : "false");
		printf("supports_av1=%s\n",
		       caps.supports_av1 ? "true" : "false");
	}

	factory->lpVtbl->Release(factory);
	return 0;
}
