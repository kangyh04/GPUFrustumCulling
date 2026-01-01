#pragma once

#include "D3DUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

struct ObjectData
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex = 0;
	UINT ObjPad0;
	UINT ObjPad1;
	UINT ObjPad2;
	float dummy0;
	float dummy1;
	float dummy2;
	float dummy3;
};

struct PassConstants
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float Pad0 = 0.0f;
	XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// 	XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	// 	float gFogStart = 5.0f;
	// 	float gFogRange = 150.0f;
	// 	XMFLOAT2 cbPerObjectPad2;

	Light Lights[MaxLights];
};

struct MaterialData
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 64.0f;

	XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT MaterialPad0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};

struct Vertex
{
	Vertex() = default;
	Vertex(float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
		: Pos(x, y, z), Normal(nx, ny, nz), TexC(u, v)
	{
	}
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;
};

struct PointVertex
{
	PointVertex() = default;
	PointVertex(float x, float y, float z, float u, float v)
		: Pos(x, y, z), Size(u, v)
	{
	}
	XMFLOAT3 Pos;
	XMFLOAT2 Size;
};

struct FrameResource
{
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	unique_ptr<UploadBuffer<ObjectData>> ObjectCB = nullptr;
	unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	UINT64 Fence = 0;
};
