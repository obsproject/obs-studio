#include <d3d12-common.hpp>
#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {
SamplerDesc::SamplerDesc(D3D12DeviceInstance *DeviceInstance) : m_DeviceInstance(DeviceInstance)
{
	Sampler.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	Filter = D3D12_FILTER_ANISOTROPIC;
	AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	MipLODBias = 0.0f;
	MaxAnisotropy = 16;
	ComparisonFunc = D3D12_COMPARISON_FUNC(0);
	BorderColor[0] = 1.0f;
	BorderColor[1] = 1.0f;
	BorderColor[2] = 1.0f;
	BorderColor[3] = 1.0f;
	MinLOD = 0.0f;
	MaxLOD = D3D12_FLOAT32_MAX;
}

SamplerDesc::~SamplerDesc()
{
	if (Sampler.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		Sampler.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
}

void SamplerDesc::SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode)
{
	AddressU = AddressMode;
	AddressV = AddressMode;
	AddressW = AddressMode;
}

void SamplerDesc::SetBorderColor(Color Border)
{
	BorderColor[0] = Border.x;
	BorderColor[1] = Border.y;
	BorderColor[2] = Border.z;
	BorderColor[3] = Border.w;
}

void SamplerDesc::CreateDescriptor(void)
{
	Sampler = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_DeviceInstance->GetDevice()->CreateSampler(this, Sampler);
}

void SamplerDesc::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	if (Handle.ptr == 0 || Handle.ptr == -1) {
		throw HRError("Invalid descriptor handle for sampler creation");
	}
	m_DeviceInstance->GetDevice()->CreateSampler(this, Handle);
}
} // namespace D3D12Graphics
