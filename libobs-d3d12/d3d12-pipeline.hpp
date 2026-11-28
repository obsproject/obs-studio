#pragma once

#include <d3d12-common.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;
class RootSignature;

class PSO {
public:
	PSO(D3D12DeviceInstance *DeviceInstance, const wchar_t *Name)
		: m_DeviceInstance(DeviceInstance),
		  m_Name(Name),
		  m_RootSignature(nullptr),
		  m_PSO(nullptr)
	{
	}

	void SetRootSignature(const RootSignature &BindMappings) { m_RootSignature = &BindMappings; }

	const RootSignature &GetRootSignature(void) const { return *m_RootSignature; }

	ID3D12PipelineState *GetPipelineStateObject(void) const { return m_PSO; }

protected:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	const wchar_t *m_Name;
	const RootSignature *m_RootSignature;
	ID3D12PipelineState *m_PSO;
};

class GraphicsPSO : public PSO {
public:
	// Start with empty state
	GraphicsPSO(D3D12DeviceInstance *DeviceInstance, const wchar_t *Name = L"Unnamed Graphics PSO");

	void SetBlendState(const D3D12_BLEND_DESC &BlendDesc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC &RasterizerDesc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC &DepthStencilDesc);
	void SetSampleMask(UINT SampleMask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
	void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1,
				   UINT MsaaQuality = 0);
	void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT *RTVFormats, DXGI_FORMAT DSVFormat,
				    UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs);
	void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

	// These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
	void SetVertexShader(const void *Binary, size_t Size);
	void SetPixelShader(const void *Binary, size_t Size);
	void SetGeometryShader(const void *Binary, size_t Size);
	void SetHullShader(const void *Binary, size_t Size);
	void SetDomainShader(const void *Binary, size_t Size);

	void SetVertexShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetPixelShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetGeometryShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetHullShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetDomainShader(const D3D12_SHADER_BYTECODE &Binary);

	// Perform validation and compute a hash value for fast state block comparisons
	void Finalize();

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
	std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
};

} // namespace D3D12Graphics
