#include <util/platform.h>
#include <util/util.hpp>
#include <util/pipe.h>
#include <util/dstr.h>
#include <obs.h>
#include <memory>
#include <vector>
#include <stdarg.h>
#include <unordered_map>
#include <mutex>
#include <atomic>

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <util/windows/device-enum.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>

#define AMFOBS_ENCODER "amf_encoder"
#define AMFOBS_ENCODER_NAME "amf_windows_impl"

#include "amf_gpu.hpp"

using namespace amf;

//------------------------------------------------------------------------------------------------------
#define AMFOBS_RETURN_IF_HR_FAILED(hr, _ret, format, ...) \
	AMFOBS_RETURN_IF_FALSE((SUCCEEDED(hr)), _ret, format, ##__VA_ARGS__)

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID, void **);

#define AMD_VENDOR_ID 0x1002

//------------------------------------------------------------------------------------------------------
class amf_windows_impl : public amf_gpu, public amf::AMFSurfaceObserver {
public:
	amf_windows_impl() {}
	~amf_windows_impl() { terminate(); }

	// amf_gpu interface
	AMF_RESULT init(amf::AMFContext *context, uint32_t adapter) override;
	AMF_RESULT terminate() override;
	AMF_RESULT copy_obs_to_amf(struct encoder_texture *texture, uint64_t lock_key, uint64_t *next_key,
				   amf::AMF_SURFACE_FORMAT format, int width, int height,
				   amf::AMFSurface **surface) override;
	AMF_RESULT get_luid(uint64_t *luid) override;
	AMF_RESULT get_vendor(uint32_t *vendor) override;

private:
	// AMFSurfaceObserver interface
	virtual void AMF_STD_CALL OnSurfaceDataRelease(amf::AMFSurface *pSurface);

	// helpers
	AMF_RESULT init_d3d11(uint32_t adapterIndex);

	AMF_RESULT add_output_tex(ComPtr<ID3D11Texture2D> &output_tex, ID3D11Texture2D *from);
	bool get_available_tex(ComPtr<ID3D11Texture2D> &output_tex);
	AMF_RESULT get_output_tex(ComPtr<ID3D11Texture2D> &output_tex, ID3D11Texture2D *from);
	AMF_RESULT get_tex_from_handle(uint32_t handle, IDXGIKeyedMutex **km_out, ID3D11Texture2D **tex_out);

	amf::AMFContext2Ptr amf_context;
	ComPtr<IDXGIAdapter> dx_adapter;
	ComPtr<ID3D11Device> dx_device;
	ComPtr<ID3D11DeviceContext> dx_context;

	struct handle_tex {
		uint32_t handle;
		ComPtr<ID3D11Texture2D> tex;
		ComPtr<IDXGIKeyedMutex> km;
	};
	using d3dtex_t = ComPtr<ID3D11Texture2D>;
	std::mutex textures_mutex;

	std::vector<handle_tex> input_textures;
	std::vector<d3dtex_t> available_textures;
	std::unordered_map<AMFSurface *, d3dtex_t> active_textures;

	std::atomic<bool> destroying = false;
};
//-----------------------------------------------------------------------------------------------
amf_gpu *amf_gpu::create()
{
	return new amf_windows_impl();
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::init(amf::AMFContext *context, uint32_t adapter)
{
	AMFOBS_RETURN_IF_FALSE(context != nullptr, AMF_INVALID_ARG, "init: invalid input parameter");
	AMF_RESULT res;
	res = init_d3d11(adapter);
	AMFOBS_RETURN_IF_FAILED(res, "init: init_d3d11() failed with res = %d", res);

	res = context->InitDX11(dx_device, AMF_DX11_1);
	if (res != AMF_OK)
		res = AMF_NOT_SUPPORTED;
	AMFOBS_RETURN_IF_FAILED(res, "init: AMFContext::InitDX11() failed with res = %d", res);

	amf_context = amf::AMFContext2Ptr(context);
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::terminate()
{
	// cleanup
	input_textures.clear();
	available_textures.clear();
	active_textures.clear();

	dx_context = nullptr;
	dx_device = nullptr;
	dx_adapter = nullptr;
	amf_context = nullptr;

	destroying.store(true);

	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
static HMODULE get_lib(const char *lib)
{
	HMODULE mod = GetModuleHandleA(lib);
	if (mod)
		return mod;

	return LoadLibraryA(lib);
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::init_d3d11(uint32_t adapterIndex)
{
	HMODULE dxgi = get_lib("DXGI.dll");
	HMODULE d3d11 = get_lib("D3D11.dll");
	CREATEDXGIFACTORY1PROC create_dxgi;
	PFN_D3D11_CREATE_DEVICE create_device;
	ComPtr<IDXGIFactory> factory;
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGIAdapter> adapter;
	DXGI_ADAPTER_DESC desc;
	HRESULT hr;

	AMFOBS_RETURN_IF_FALSE(dxgi != nullptr || d3d11 != nullptr, AMF_FAIL,
			       "init_d3d11: Couldn't get D3D11/DXGI libraries? That definitely shouldn't be possible.");

	create_dxgi = (CREATEDXGIFACTORY1PROC)GetProcAddress(dxgi, "CreateDXGIFactory1");
	create_device = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11, "D3D11CreateDevice");

	AMFOBS_RETURN_IF_FALSE(create_dxgi != nullptr || create_device != nullptr, AMF_FAIL,
			       "init_d3d11: Failed to load D3D11/DXGI procedures");

	hr = create_dxgi(__uuidof(IDXGIFactory2), (void **)&factory);
	AMFOBS_RETURN_IF_HR_FAILED(hr, AMF_FAIL, "init_d3d11: CreateDXGIFactory1 failed: 0x%lX", hr);

	hr = factory->EnumAdapters(adapterIndex, &adapter);
	AMFOBS_RETURN_IF_HR_FAILED(hr, AMF_FAIL, "init_d3d11: EnumAdapters(%d) failed: 0x%lX", adapter, hr);

	adapter->GetDesc(&desc);

	AMFOBS_RETURN_IF_FALSE(desc.VendorId == AMD_VENDOR_ID, AMF_FAIL,
			       "init_d3d11: Seems somehow AMF is trying to initialize on a non-AMD adapter");

	hr = create_device(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device,
			   nullptr, &context);
	AMFOBS_RETURN_IF_HR_FAILED(hr, AMF_FAIL, "init_d3d11: D3D11CreateDevice for adapter %d failed: 0x%lX", adapter,
				   hr);

	dx_adapter = adapter;
	dx_device = device;
	dx_context = context;
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::add_output_tex(ComPtr<ID3D11Texture2D> &output_tex, ID3D11Texture2D *from)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc;
	from->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = 0;

	hr = dx_device->CreateTexture2D(&desc, nullptr, &output_tex);
	AMFOBS_RETURN_IF_HR_FAILED(hr, AMF_FAIL, "add_output_tex: CreateTexture2D failed to create texture: 0x%lX", hr);
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
bool amf_windows_impl::get_available_tex(ComPtr<ID3D11Texture2D> &output_tex)
{
	std::scoped_lock lock(textures_mutex);
	if (available_textures.size()) {
		output_tex = available_textures.back();
		available_textures.pop_back();
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::get_output_tex(ComPtr<ID3D11Texture2D> &output_tex, ID3D11Texture2D *from)
{
	if (!get_available_tex(output_tex))
		return add_output_tex(output_tex, from);
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::get_tex_from_handle(uint32_t handle, IDXGIKeyedMutex **km_out, ID3D11Texture2D **tex_out)
{
	ComPtr<ID3D11Texture2D> tex;
	HRESULT hr;

	for (size_t i = 0; i < input_textures.size(); i++) {
		struct handle_tex &ht = input_textures[i];
		if (ht.handle == handle) {
			ht.km.CopyTo(km_out);
			ht.tex.CopyTo(tex_out);
			return AMF_OK;
		}
	}

	hr = dx_device->OpenSharedResource((HANDLE)(uintptr_t)handle, __uuidof(ID3D11Resource), (void **)&tex);
	AMFOBS_RETURN_IF_HR_FAILED(hr, AMF_FAIL, "OpenSharedResource() failed to open texture: 0x%lX", hr);

	ComQIPtr<IDXGIKeyedMutex> km(tex);
	AMFOBS_RETURN_IF_FALSE(km != nullptr, AMF_FAIL, "QueryInterface(IDXGIKeyedMutex) failed");

	tex->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);

	struct handle_tex new_ht = {handle, tex, km};
	input_textures.push_back(std::move(new_ht));

	*km_out = km.Detach();
	*tex_out = tex.Detach();
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::copy_obs_to_amf(struct encoder_texture *texture, uint64_t lock_key, uint64_t *next_key,
					     amf::AMF_SURFACE_FORMAT /*format*/, int /*width*/, int /*height*/,
					     amf::AMFSurface **surface)
{
	AMFOBS_RETURN_IF_FALSE(amf_context != nullptr, AMF_NOT_INITIALIZED, "copy_obs_to_amf: Not initialized");

	uint32_t handle = texture->handle;
	ComPtr<ID3D11Texture2D> output_tex;
	ComPtr<ID3D11Texture2D> input_tex;
	ComPtr<IDXGIKeyedMutex> km;
	AMF_RESULT res;

	if (handle == GS_INVALID_HANDLE) {
		*next_key = lock_key;
		AMFOBS_RETURN_IF_FALSE(handle != GS_INVALID_HANDLE, AMF_FAIL,
				       "copy_obs_to_amf: Encode failed: bad texture handle");
	}

	get_tex_from_handle(handle, &km, &input_tex);
	AMFOBS_RETURN_IF_FALSE(input_tex != nullptr, AMF_FAIL, "copy_obs_to_amf: get_tex_from_handle() failed");

	get_output_tex(output_tex, input_tex);
	AMFOBS_RETURN_IF_FALSE(output_tex != nullptr, AMF_FAIL, "copy_obs_to_amf: get_output_tex() failed");

	km->AcquireSync(lock_key, INFINITE);
	dx_context->CopyResource((ID3D11Resource *)output_tex.Get(), (ID3D11Resource *)input_tex.Get());
	dx_context->Flush();
	km->ReleaseSync(*next_key);

	res = amf_context->CreateSurfaceFromDX11Native(output_tex, surface, this);
	AMFOBS_RETURN_IF_FAILED(res, "copy_obs_to_amf: AMFContext::CreateSurfaceFromDX11Native() failed with res = %d",
				res);
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
void AMF_STD_CALL amf_windows_impl::OnSurfaceDataRelease(amf::AMFSurface *surf)
{
	if (destroying.load())
		return;
	std::scoped_lock lock(textures_mutex);
	auto it = active_textures.find(surf);
	if (it != active_textures.end()) {
		available_textures.push_back(it->second);
		active_textures.erase(it);
	}
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::get_luid(uint64_t *luid)
{
	AMFOBS_RETURN_IF_FALSE(dx_adapter != nullptr, AMF_NOT_INITIALIZED, "get_luid: Not initialized");
	DXGI_ADAPTER_DESC desc = {};
	dx_adapter->GetDesc(&desc);
	*luid = *((uint64_t *)&desc.AdapterLuid);
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
AMF_RESULT amf_windows_impl::get_vendor(uint32_t *vendor)
{
	AMFOBS_RETURN_IF_FALSE(dx_adapter != nullptr, AMF_NOT_INITIALIZED, "get_vendor: Not initialized");
	DXGI_ADAPTER_DESC desc = {};
	dx_adapter->GetDesc(&desc);
	*vendor = (uint32_t)desc.VendorId;
	return AMF_OK;
}
//------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------
