#pragma once

// Code from the DirectX-Graphics-Samples and DirectXTK12 repositories
// See: https://github.com/microsoft/DirectX-Graphics-Samples.git
// See: https://github.com/microsoft/DirectXTK12.git

// This is a standalone D3D12 public module; do not include libobs-related header files.

#include <d3d12-gpubuffer.hpp>
#include <d3d12-linearallocator.hpp>
#include <d3d12-rootsignature.hpp>
#include <d3d12-descriptorallocator.hpp>
#include <d3d12-dynamic-descriptorheap.hpp>
#include <d3d12-commandqueue.hpp>
#include <d3d12-pipeline.hpp>
#include <d3d12-context.hpp>

#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace D3D12Graphics {

class RootParameter;
class RootSignature;
class DescriptorAllocator;
class DescriptorHandle;
class DescriptorHandleCache;
class DynamicDescriptorHeap;
class ContextManager;
class CommandContext;
class GraphicsContext;
class ComputeContext;

class HagsStatus;

struct MonitorColorInfo {
	bool m_HDR;
	UINT m_BitsPerColor;
	ULONG m_SDRWhiteNits;

	MonitorColorInfo(bool HDR, int BitsPerColor, ULONG SDRWhiteNits)
		: m_HDR(HDR),
		  m_BitsPerColor(BitsPerColor),
		  m_SDRWhiteNits(SDRWhiteNits)
	{
	}
};

class D3D12DeviceInstance {
public:
	D3D12DeviceInstance();
	void Initialize(int32_t adaptorIndex);
	void Uninitialize();

	ID3D12Device *GetDevice();
	IDXGIAdapter1 *GetAdapter();
	IDXGIFactory6 *GetDxgiFactory();
	CommandListManager &GetCommandManager();
	ContextManager &GetContextManager();

	std::map<size_t, ComPtr<ID3D12RootSignature>> &GetRootSignatureHashMap();
	LinearAllocatorPageManager *GetPageManager(LinearAllocatorType AllocatorType);
	std::map<size_t, ComPtr<ID3D12PipelineState>> &GetGraphicsPSOHashMap();
	std::map<size_t, ComPtr<ID3D12PipelineState>> &GetComputePSOHashMap();
	DescriptorAllocator *GetDescriptorAllocator();

	DescriptorAllocator m_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		{this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
		{this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER},
		{this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV},
		{this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV}};
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1);

	ID3D12DescriptorHeap *RequestCommonHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

	ID3D12DescriptorHeap *RequestDynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
	void DiscardDynamicDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset,
					   const std::vector<ID3D12DescriptorHeap *> &UsedHeaps);

	GraphicsContext *GetNewGraphicsContext(const std::wstring &ID = L"");
	ComputeContext *GetNewComputeContext(const std::wstring &ID = L"", bool Async = false);

	void InitializeTexture(GpuResource &Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	void InitializeBuffer(GpuBuffer &Dest, const void *Data, size_t NumBytes, size_t DestOffset = 0);
	void InitializeBuffer(GpuBuffer &Dest, const UploadBuffer &Src, size_t SrcOffset, size_t NumBytes = -1,
			      size_t DestOffset = 0);
	void InitializeTextureArraySlice(GpuResource &Dest, UINT SliceIndex, GpuResource &Src);
	bool IsNV12TextureSupported() const;
	bool IsP010TextureSupported() const;
	bool FastClearSupported() const;

	static std::optional<HagsStatus> GetAdapterHagsStatus(const DXGI_ADAPTER_DESC *desc);
	static void EnumD3DAdapters(bool (*callback)(void *, const char *, uint32_t), void *param);
	static bool GetMonitorTarget(const MONITORINFOEX &info, DISPLAYCONFIG_TARGET_DEVICE_NAME &target);
	static bool GetOutputDesc1(IDXGIOutput *const output, DXGI_OUTPUT_DESC1 *desc1);
	static HRESULT GetPathInfo(_In_ PCWSTR pszDeviceName, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfo);
	static HRESULT GetPathInfo(HMONITOR hMonitor, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfo);
	static ULONG GetSdrMaxNits(HMONITOR monitor);
	static MonitorColorInfo GetMonitorColorInfo(HMONITOR hMonitor);
	static void PopulateMonitorIds(HMONITOR handle, char *id, char *alt_id, size_t capacity);
	static void LogAdapterMonitors(IDXGIAdapter1 *adapter);
	static void LogD3DAdapters();
	static bool IsInternalVideoOutput(const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY VideoOutputTechnologyType);

private:
	void EnableDebugLayer();
	void EnableDebugInofQueue();
	void CheckFeatureSupports();
	void CreateD3DAdapterAndDevice(uint32_t index);

	const wchar_t *GPUVendorToString(uint32_t vendorID);
	uint32_t GetVendorIdFromDevice(ID3D12Device *pDevice);
	bool IsDeviceNvidia(ID3D12Device *pDevice);
	bool IsDeviceAMD(ID3D12Device *pDevice);
	bool IsDeviceIntel(ID3D12Device *pDevice);
	// Check adapter support for DirectX Raytracing.
	bool IsDirectXRaytracingSupported(ID3D12Device *testDevice);

private:
	std::vector<ComPtr<ID3D12DescriptorHeap>> m_DescriptorHeapPool;

	ComPtr<ID3D12Device> m_Device = nullptr;
	ComPtr<IDXGIAdapter1> m_Adapter = nullptr;
	ComPtr<IDXGIFactory6> m_DxgiFactory = nullptr;

	std::unique_ptr<CommandListManager> m_CommandManager;
	std::unique_ptr<ContextManager> m_ContextManager;

	std::map<size_t, ComPtr<ID3D12RootSignature>> m_RootSignatureHashMap;

	std::vector<ComPtr<ID3D12DescriptorHeap>> m_DynamicDescriptorHeapPool[2];
	std::queue<std::pair<uint64_t, ID3D12DescriptorHeap *>> m_DynamicRetiredDescriptorHeaps[2];
	std::queue<ID3D12DescriptorHeap *> m_DynamicAvailableDescriptorHeaps[2];

	std::unique_ptr<LinearAllocatorPageManager> m_PageManager[2];

	std::map<size_t, ComPtr<ID3D12PipelineState>> m_GraphicsPSOHashMap;
	std::map<size_t, ComPtr<ID3D12PipelineState>> m_ComputePSOHashMap;

	bool m_TypedUAVLoadSupport_R11G11B10_FLOAT = false;
	bool m_TypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
	bool m_NV12Supported = false;
	bool m_P010Supported = false;
	bool m_FastClearSupported = false;
};

} // namespace D3D12Graphics
