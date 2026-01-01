#pragma once

#include "D3DUtil.h"

enum class CubeFace : int
{
	CF_POSITIVE_X = 0,
	CF_NEGATIVE_X = 1,
	CF_POSITIVE_Y = 2,
	CF_NEGATIVE_Y = 3,
	CF_POSITIVE_Z = 4,
	CF_NEGATIVE_Z = 5
};

class CubeRenderTarget
{
public:
	CubeRenderTarget(ID3D12Device* device,
		UINT width,
		UINT height,
		DXGI_FORMAT format);
	~CubeRenderTarget() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(int face);

	D3D12_VIEWPORT Viewport() const;
	D3D12_RECT ScissorRect() const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuRtv[6];

	ComPtr<ID3D12Resource> mCubeMap = nullptr;
};
