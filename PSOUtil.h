#pragma once
#include "D3DUtil.h"

class PSOUtil
{
public:
	static D3D12_GRAPHICS_PIPELINE_STATE_DESC MakeOpaquePSODesc(
		ID3DBlob* vs,
		ID3DBlob* ps,
		vector<D3D12_INPUT_ELEMENT_DESC> inputLayout,
		ID3D12RootSignature* rootSignature,
		DXGI_FORMAT backBufferFormat,
		DXGI_FORMAT depthStencilFormat,
		bool msaaState,
		int msaaQuality)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

		psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
		psoDesc.pRootSignature = rootSignature;
		psoDesc.VS =
		{
			reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
			vs->GetBufferSize()
		};
		psoDesc.PS =
		{
			reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
			ps->GetBufferSize()
		};
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = backBufferFormat;
		psoDesc.SampleDesc.Count = msaaState ? 4 : 1;
		psoDesc.SampleDesc.Quality = msaaState ? (msaaQuality - 1) : 0;
		psoDesc.DSVFormat = depthStencilFormat;
		return psoDesc;
	}
};
