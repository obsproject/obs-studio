#pragma once

#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <unordered_map>
#include <map>
#include <optional>

#include <util/platform.h>
#include <windows.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/HRError.hpp>
#include <d3d12.h>

#define PERF_GRAPH_ERROR uint32_t(0xFFFFFFFF)
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#define DEFAULT_ALIGN 256

static const uint32_t kMaxNumDescriptors = 256;
static const uint32_t kMaxNumDescriptorTables = 16;

static const uint32_t vendorID_Nvidia = 0x10DE;
static const uint32_t vendorID_AMD = 0x1002;
static const uint32_t vendorID_Intel = 0x8086;

typedef uint32_t GraphHandle;

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

namespace D3D12Graphics {

class D3D12DeviceInstance;

enum LinearAllocatorType {
	kInvalidAllocator = -1,
	kGpuExclusive = 0, // DEFAULT   GPU-writeable (via UAV)
	kCpuWritable = 1,  // UPLOAD CPU-writeable (but write combined)
	kNumAllocatorTypes
};

enum {
	kGpuAllocatorPageSize = 0x10000, // 64K
	kCpuAllocatorPageSize = 0x200000 // 2MB
};

struct Color {
	union {
		struct {
			float x, y, z, w;
		};
		float ptr[4];
	};
};

inline size_t HashRange(const uint32_t *const Begin, const uint32_t *const End, size_t Hash)
{
	// An inexpensive hash for CPUs lacking SSE4.2
	for (const uint32_t *Iter = Begin; Iter < End; ++Iter)
		Hash = 16777619U * Hash ^ *Iter;

	return Hash;
}

template<typename T> inline size_t HashState(const T *StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
{
	static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
	return HashRange((uint32_t *)StateDesc, (uint32_t *)(StateDesc + Count), Hash);
}

template<typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
{
	return (T)(((size_t)value + mask) & ~mask);
}

template<typename T> __forceinline T AlignDownWithMask(T value, size_t mask)
{
	return (T)((size_t)value & ~mask);
}

template<typename T> __forceinline T AlignUp(T value, size_t alignment)
{
	return AlignUpWithMask(value, alignment - 1);
}

template<typename T> __forceinline T AlignDown(T value, size_t alignment)
{
	return AlignDownWithMask(value, alignment - 1);
}

template<typename T> __forceinline bool IsAligned(T value, size_t alignment)
{
	return 0 == ((size_t)value & (alignment - 1));
}

template<typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
{
	return (T)((value + alignment - 1) / alignment);
}

static constexpr double DoubleTriangleArea(double ax, double ay, double bx, double by, double cx, double cy)
{
	return ax * (by - cy) + bx * (cy - ay) + cx * (ay - by);
}

static inline double to_GiB(size_t bytes)
{
	return static_cast<double>(bytes) / (1 << 30);
}

static size_t BitsPerPixel(_In_ DXGI_FORMAT fmt)
{
	switch (fmt) {
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
		return 32;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		return 24;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 16;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
		return 12;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;

	default:
		return 0;
	}
}

static UINT BytesPerPixel(DXGI_FORMAT Format)
{
	return (UINT)BitsPerPixel(Format) / 8;
}

inline UINT64 GetRequiredIntermediateSize(_In_ ID3D12Resource *pDestinationResource,
					  _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
					  _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource)
						  UINT NumSubresources) noexcept
{
	auto Desc = pDestinationResource->GetDesc();
	UINT64 RequiredSize = 0;

	ID3D12Device *pDevice = nullptr;
	pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void **>(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr,
				       &RequiredSize);
	pDevice->Release();

	return RequiredSize;
}

struct CD3DX12_RANGE : public D3D12_RANGE {
	CD3DX12_RANGE() = default;
	explicit CD3DX12_RANGE(const D3D12_RANGE &o) noexcept : D3D12_RANGE(o) {}
	CD3DX12_RANGE(SIZE_T begin, SIZE_T end) noexcept
	{
		Begin = begin;
		End = end;
	}
};

struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES {
	CD3DX12_HEAP_PROPERTIES() = default;
	explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES &o) noexcept : D3D12_HEAP_PROPERTIES(o) {}
	CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY cpuPageProperty, D3D12_MEMORY_POOL memoryPoolPreference,
				UINT creationNodeMask = 1, UINT nodeMask = 1) noexcept
	{
		Type = D3D12_HEAP_TYPE_CUSTOM;
		CPUPageProperty = cpuPageProperty;
		MemoryPoolPreference = memoryPoolPreference;
		CreationNodeMask = creationNodeMask;
		VisibleNodeMask = nodeMask;
	}
	explicit CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE type, UINT creationNodeMask = 1, UINT nodeMask = 1) noexcept
	{
		Type = type;
		CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		CreationNodeMask = creationNodeMask;
		VisibleNodeMask = nodeMask;
	}
	bool IsCPUAccessible() const noexcept
	{
		return Type == D3D12_HEAP_TYPE_UPLOAD || Type == D3D12_HEAP_TYPE_READBACK ||
		       (Type == D3D12_HEAP_TYPE_CUSTOM && (CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE ||
							   CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK));
	}
};

inline bool operator==(const D3D12_HEAP_PROPERTIES &l, const D3D12_HEAP_PROPERTIES &r) noexcept
{
	return l.Type == r.Type && l.CPUPageProperty == r.CPUPageProperty &&
	       l.MemoryPoolPreference == r.MemoryPoolPreference && l.CreationNodeMask == r.CreationNodeMask &&
	       l.VisibleNodeMask == r.VisibleNodeMask;
}

inline bool operator!=(const D3D12_HEAP_PROPERTIES &l, const D3D12_HEAP_PROPERTIES &r) noexcept
{
	return !(l == r);
}

struct CD3DX12_SHADER_BYTECODE : public D3D12_SHADER_BYTECODE {
	CD3DX12_SHADER_BYTECODE() = default;
	explicit CD3DX12_SHADER_BYTECODE(const D3D12_SHADER_BYTECODE &o) noexcept : D3D12_SHADER_BYTECODE(o) {}
	CD3DX12_SHADER_BYTECODE(_In_ ID3DBlob *pShaderBlob) noexcept
	{
		pShaderBytecode = pShaderBlob->GetBufferPointer();
		BytecodeLength = pShaderBlob->GetBufferSize();
	}
	CD3DX12_SHADER_BYTECODE(const void *_pShaderBytecode, SIZE_T bytecodeLength) noexcept
	{
		pShaderBytecode = _pShaderBytecode;
		BytecodeLength = bytecodeLength;
	}
};

struct CD3DX12_TEXTURE_COPY_LOCATION : public D3D12_TEXTURE_COPY_LOCATION {
	CD3DX12_TEXTURE_COPY_LOCATION() = default;
	explicit CD3DX12_TEXTURE_COPY_LOCATION(const D3D12_TEXTURE_COPY_LOCATION &o) noexcept
		: D3D12_TEXTURE_COPY_LOCATION(o)
	{
	}
	CD3DX12_TEXTURE_COPY_LOCATION(_In_ ID3D12Resource *pRes) noexcept
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		PlacedFootprint = {};
	}
	CD3DX12_TEXTURE_COPY_LOCATION(_In_ ID3D12Resource *pRes,
				      D3D12_PLACED_SUBRESOURCE_FOOTPRINT const &Footprint) noexcept
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		PlacedFootprint = Footprint;
	}
	CD3DX12_TEXTURE_COPY_LOCATION(_In_ ID3D12Resource *pRes, UINT Sub) noexcept
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		PlacedFootprint = {};
		SubresourceIndex = Sub;
	}
};

struct CD3DX12_RECT : public D3D12_RECT {
	CD3DX12_RECT() = default;
	explicit CD3DX12_RECT(const D3D12_RECT &o) noexcept : D3D12_RECT(o) {}
	explicit CD3DX12_RECT(LONG Left, LONG Top, LONG Right, LONG Bottom) noexcept
	{
		left = Left;
		top = Top;
		right = Right;
		bottom = Bottom;
	}
};

class SamplerDesc : public D3D12_SAMPLER_DESC {
public:
	SamplerDesc(D3D12DeviceInstance *DeviceInstance);
	~SamplerDesc();
	void SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode);
	void SetBorderColor(Color Border);

	void CreateDescriptor(void);
	void CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle);

	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE Sampler;
};

} // namespace D3D12Graphics
