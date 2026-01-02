#pragma once
#include <memory>
#include "UploadBuffer.h"

struct SceneObjectData
{
	XMFLOAT4 WorldPosition;
	XMFLOAT3 Size;
	UINT pad0;
};

struct IndirectCommand
{
	UINT InstanceId;
	UINT pad0;
	UINT pad1;
	UINT pad2;
};

struct Plane
{
	XMFLOAT3 Normal;
	float Distance;
};

struct CountCommand
{
	UINT Count;
	UINT pad0;
	UINT pad1;
	UINT pad2;
};

class GPUFrustumCulling
{
public:
	void BuildResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, vector<IndirectCommand> sceneObjectCommand);
	void UpdateSceneObjectBuffer(ID3D12Device* device, const vector<SceneObjectData>& sceneObjects);
	void UpdateFrustumPlaneBuffer(ID3D12Device* device, const XMMATRIX viewProj);
	void CullSceneObjects(
		ID3D12GraphicsCommandList* cmdList,
		UINT sceneObjectCount,
		ID3D12Resource* indirectInputCommandBuffer,
		ID3D12Resource* indirectOutputCommandBuffer);
	void ExtractPlanes(const XMMATRIX& viewProjMatrix, vector<Plane>& planes);
private:
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	unique_ptr<UploadBuffer<SceneObjectData>> mSceneObjectBuffer;
	unique_ptr<UploadBuffer<Plane>> mFrustumPlaneBuffer;
	unique_ptr<UploadBuffer<CountCommand>> mCountBuffer;
	ComPtr<ID3D12Resource> mIndirectInputCommandBuffer;
	ComPtr<ID3D12Resource> mIndirectInputUploadBuffer;
	ComPtr<ID3D12Resource> mIndirectOutputCommandBuffer;

	constexpr static UINT PlaneCount = 6;
};
