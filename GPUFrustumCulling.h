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
	D3D12_VERTEX_BUFFER_VIEW vertexView;
	D3D12_INDEX_BUFFER_VIEW indexView;
	D3D12_DRAW_INDEXED_ARGUMENTS drawArgument;
	UINT pad0;
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
	void Build(ID3D12Device* device, ID3D12RootSignature* graphicsRootSig);
	void UpdateIndirectCommand(const vector<IndirectCommand> commands);
	void CullSceneObjects(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const XMMATRIX& viewProjMatrix,
		const vector<SceneObjectData>& sceneObjects);

	ID3D12CommandSignature* GetCommandSignature()
	{
		return mCommandSignature.Get();
	}

	ID3D12Resource* GetVisibilityResource()
	{
		return mIndirectOutputVisibilityBuffer.Get();
	}

	ComPtr<ID3D12CommandAllocator> CommandAllocator()
	{
		return mCommandAllocator;
	}

	ID3D12Resource* GetIndirectBuffer()
	{
		return mIndirectBuffer.Get();
	}

	CountCommand* CountBuffer()
	{
		return mCountBuffer.get();
	}

private:
	void BuildRootSignature(ID3D12Device* device);
	void BuildComputeShader();
	void BuildCommandSignature(ID3D12Device* device, ID3D12RootSignature* graphicsRootSignature);
	void BuildIndirectCommandBuffer(ID3D12Device* device);
	void BuildPSO(ID3D12Device* device);

	void UpdateSceneObjectBuffer(const vector<SceneObjectData>& sceneObjects);
	void UpdateFrustumPlaneBuffer(const XMMATRIX& viewProj);

	void ExtractPlanes(const XMMATRIX& viewProjMatrix, vector<Plane>& planes);

public:
	static constexpr UINT MaximumCommandAmount = 16;
	static constexpr UINT MaximumObjectAmountPerCommand = 128;
	static constexpr UINT MaximumObjectAmount = MaximumCommandAmount * MaximumObjectAmountPerCommand;
	static constexpr UINT CommandSizePerFrame = MaximumObjectAmount * sizeof(IndirectCommand);
	static constexpr UINT CommandBufferCounterOffset = D3DUtil::AlignForUAVCounter(CommandSizePerFrame);

private:
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	unique_ptr<UploadBuffer<SceneObjectData>> mSceneObjectBuffer;
	unique_ptr<UploadBuffer<Plane>> mFrustumPlaneBuffer;
	unique_ptr<UploadBuffer<IndirectCommand>> mIndirectResetBuffer;
	ComPtr<ID3D12Resource> mIndirectBuffer;
	unique_ptr<CountCommand> mCountBuffer;

	ComPtr<ID3D12CommandSignature> mCommandSignature;

	ComPtr<ID3D12Resource> mIndirectOutputVisibilityBuffer;

	ComPtr<ID3D12RootSignature> mRootSignature;

	ComPtr<ID3DBlob> mCSShaderByteCode;

	ComPtr<ID3D12PipelineState> mPSO;

	static constexpr UINT PlaneCount = 6;
	static constexpr UINT ThreadGroupSize = 128;
};
