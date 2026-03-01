#include <d3d12-deviceinstance.hpp>

#include <thread>

namespace D3D12Graphics {
GraphicsPSO::GraphicsPSO(D3D12DeviceInstance *DeviceInstance, const wchar_t *Name) : PSO(DeviceInstance, Name)
{
	ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
	m_PSODesc.NodeMask = 1;
	m_PSODesc.SampleMask = 0xFFFFFFFFu;
	m_PSODesc.SampleDesc.Count = 1;
	m_PSODesc.InputLayout.NumElements = 0;
}

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC &BlendDesc)
{
	m_PSODesc.BlendState = BlendDesc;
}

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC &RasterizerDesc)
{
	m_PSODesc.RasterizerState = RasterizerDesc;
}

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC &DepthStencilDesc)
{
	m_PSODesc.DepthStencilState = DepthStencilDesc;
}

void GraphicsPSO::SetSampleMask(UINT SampleMask)
{
	m_PSODesc.SampleMask = SampleMask;
}

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	if (TopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED) {
		throw HRError("GraphicsPSO: Can't draw with undefined topology.");
	}
	m_PSODesc.PrimitiveTopologyType = TopologyType;
}

void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
	SetRenderTargetFormats(0, nullptr, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
	SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT *RTVFormats, DXGI_FORMAT DSVFormat,
					 UINT MsaaCount, UINT MsaaQuality)
{
	if (!(NumRTVs == 0 || RTVFormats != nullptr)) {
		throw HRError("GraphicsPSO: RTVFormats pointer is null with non-zero NumRTVs.");
	}
	for (UINT i = 0; i < NumRTVs; ++i) {
		if (RTVFormats[i] == DXGI_FORMAT_UNKNOWN) {
			throw HRError("GraphicsPSO: RTVFormats contains an UNKNOWN format.");
		}
		m_PSODesc.RTVFormats[i] = RTVFormats[i];
	}
	for (UINT i = NumRTVs; i < m_PSODesc.NumRenderTargets; ++i)
		m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	m_PSODesc.NumRenderTargets = NumRTVs;
	m_PSODesc.DSVFormat = DSVFormat;
	m_PSODesc.SampleDesc.Count = MsaaCount;
	m_PSODesc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs)
{
	m_PSODesc.InputLayout.NumElements = NumElements;

	if (NumElements > 0) {
		D3D12_INPUT_ELEMENT_DESC *NewElements =
			(D3D12_INPUT_ELEMENT_DESC *)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
		memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
		m_InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC *)NewElements);
	} else
		m_InputLayouts = nullptr;
}

void GraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
	m_PSODesc.IBStripCutValue = IBProps;
}

void GraphicsPSO::SetVertexShader(const void *Binary, size_t Size)
{
	m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetPixelShader(const void *Binary, size_t Size)
{
	m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetGeometryShader(const void *Binary, size_t Size)
{
	m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetHullShader(const void *Binary, size_t Size)
{
	m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetDomainShader(const void *Binary, size_t Size)
{
	m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetVertexShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.VS = Binary;
}

void GraphicsPSO::SetPixelShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.PS = Binary;
}

void GraphicsPSO::SetGeometryShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.GS = Binary;
}

void GraphicsPSO::SetHullShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.HS = Binary;
}

void GraphicsPSO::SetDomainShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.DS = Binary;
}

void GraphicsPSO::Finalize()
{
	m_PSODesc.pRootSignature = m_RootSignature->GetSignature();

	m_PSODesc.InputLayout.pInputElementDescs = nullptr;
	size_t HashCode = HashState(&m_PSODesc);
	HashCode = HashState(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, HashCode);
	m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

	ComPtr<ID3D12PipelineState> PSORef = nullptr;
	bool firstCompile = false;
	{
		auto iter = m_DeviceInstance->GetGraphicsPSOHashMap().find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_DeviceInstance->GetGraphicsPSOHashMap().end()) {
			firstCompile = true;
			PSORef = m_DeviceInstance->GetGraphicsPSOHashMap()[HashCode];
		} else
			PSORef = iter->second;
	}
	if (firstCompile) {
		HRESULT hr =
			m_DeviceInstance->GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO));
		if (FAILED(hr)) {
			throw HRError("GraphicsPSO: Failed to create graphics pipeline state object.");
		}
		m_DeviceInstance->GetGraphicsPSOHashMap()[HashCode].Set(m_PSO);
		m_PSO->SetName(m_Name);
	} else {
		while (PSORef == nullptr)
			std::this_thread::yield();
		m_PSO = PSORef;
	}
}

} // namespace D3D12Graphics
