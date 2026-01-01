#pragma once

#include "MathHelper.h"
#include "UploadBuffer.h"

struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	int NumFramesDirty = gNumFrameResources;

	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;
	Material* Mat = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	BoundingBox Bounds;
	vector<Instance> Instances;

	UINT IndexCount = 0;
	UINT InstanceCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	bool Visible = true;
};

enum class RenderLayer : int
{
	Opaque = 0,
	OpaqueDynamicReflectors,
	Sky,
	Count
	// Mirrors,
	// Reflected,
	// Transparent,
	// AlphaTested,
	// AlphaTestedTreeSprites,
	// Shadow,
	// Count
};

enum class TextureLayer : int
{
	UNKNOWN = 0,
	BUFFER = 1,
	TEXTURE1D = 2,
	TEXTURE1DARRAY = 3,
	TEXTURE2D = 4,
	TEXTURE2DARRAY = 5,
	TEXTURE2DMS = 6,
	TEXTURE2DMSARRAY = 7,
	TEXTURE3D = 8,
	TEXTURECUBE = 9,
	TEXTURECUBEARRAY = 10,
	RAYTRACING_ACCELERATION_STRUCTURE = 11,
	Count
};
