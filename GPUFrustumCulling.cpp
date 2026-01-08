#include "GPUFrustumCulling.h"

void GPUFrustumCulling::Build(ID3D12Device* device, ID3D12RootSignature* graphicsRootSig)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		IID_PPV_ARGS(mCommandAllocator.GetAddressOf())));

	BuildRootSignature(device);
	BuildComputeShader();
	BuildCommandSignature(device, graphicsRootSig);
	BuildIndirectCommandBuffer(device);
	BuildPSO(device);
}

void GPUFrustumCulling::UpdateSceneObjectBuffer(const vector<SceneObjectData>& sceneObjects)
{
	for (UINT i = 0; i < sceneObjects.size(); ++i)
	{
		mSceneObjectBuffer->CopyData(i, sceneObjects[i]);
	}
}

void GPUFrustumCulling::UpdateFrustumPlaneBuffer(const XMMATRIX& viewProj)
{
	vector<Plane> frustumPlanes;
	ExtractPlanes(viewProj, frustumPlanes);
	for (UINT i = 0; i < frustumPlanes.size(); ++i)
	{
		mFrustumPlaneBuffer->CopyData(i, frustumPlanes[i]);
	}
}

void GPUFrustumCulling::UpdateIndirectCommand(const vector<IndirectCommand> commands)
{
	for (UINT i = 0; i < commands.size(); ++i)
	{
		mIndirectResetBuffer->CopyData(i, commands[i]);
	}
	mCountBuffer->Count = commands.size();
}

void GPUFrustumCulling::CullSceneObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const XMMATRIX& viewProjMatrix, const vector<SceneObjectData>& sceneObjects)
{
	UpdateSceneObjectBuffer(sceneObjects);
	UpdateFrustumPlaneBuffer(viewProjMatrix);

	cmdList->SetPipelineState(mPSO.Get());

	cmdList->SetComputeRootSignature(mRootSignature.Get());

	cmdList->SetComputeRoot32BitConstants(0, 1, reinterpret_cast<void*>(&mCountBuffer->Count), 0);

	cmdList->SetComputeRootShaderResourceView(1, mSceneObjectBuffer->Resource()->GetGPUVirtualAddress());

	cmdList->SetComputeRootShaderResourceView(2, mFrustumPlaneBuffer->Resource()->GetGPUVirtualAddress());

	auto toCopy = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectBuffer.Get(),
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		D3D12_RESOURCE_STATE_COPY_DEST);

	cmdList->ResourceBarrier(1, &toCopy);

	cmdList->CopyBufferRegion(mIndirectBuffer.Get(), 0, mIndirectResetBuffer->Resource(), 0, sizeof(IndirectCommand) * MaximumCommandAmount);

	CD3DX12_RESOURCE_BARRIER toCSState[2];

	toCSState[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	toCSState[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectOutputVisibilityBuffer.Get(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cmdList->ResourceBarrier(size(toCSState), toCSState);

	cmdList->SetComputeRootUnorderedAccessView(3, mIndirectBuffer->GetGPUVirtualAddress());

	cmdList->SetComputeRootUnorderedAccessView(4, mIndirectOutputVisibilityBuffer->GetGPUVirtualAddress());

	cmdList->Dispatch(
		(UINT)ceilf((float)sceneObjects.size() / ThreadGroupSize),
		1,
		1);

	CD3DX12_RESOURCE_BARRIER toDrawState[2];

	toDrawState[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	toDrawState[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectOutputVisibilityBuffer.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cmdList->ResourceBarrier(size(toDrawState), toDrawState);
}

void GPUFrustumCulling::ExtractPlanes(const XMMATRIX& viewProjMatrix, vector<Plane>& planes)
{
	XMFLOAT4X4 m;
	XMStoreFloat4x4(&m, XMMatrixTranspose(viewProjMatrix));

	vector<XMVECTOR> planeEquations(6);
	planeEquations[0] = XMVectorSet(
		m._14 + m._11,
		m._24 + m._21,
		m._34 + m._31,
		m._44 + m._41); // Left
	planeEquations[1] = XMVectorSet(
		m._14 - m._11,
		m._24 - m._21,
		m._34 - m._31,
		m._44 - m._41); // Right
	planeEquations[2] = XMVectorSet(
		m._14 + m._12,
		m._24 + m._22,
		m._34 + m._32,
		m._44 + m._42); // Bottom
	planeEquations[3] = XMVectorSet(
		m._14 - m._12,
		m._24 - m._22,
		m._34 - m._32,
		m._44 - m._42); // Top
	planeEquations[4] = XMVectorSet(
		m._13,
		m._23,
		m._33,
		m._43); // Near
	planeEquations[5] = XMVectorSet(
		m._14 - m._13,
		m._24 - m._23,
		m._34 - m._33,
		m._44 - m._43); // Far

	for (UINT i = 0; i < PlaneCount; ++i)
	{
		Plane plane;
		auto normalizedPlane = XMVector4Normalize(planeEquations[i]);
		XMStoreFloat3(&plane.Normal, XMVectorSetW(planeEquations[i], 0.0f));
		plane.Distance = XMVectorGetW(normalizedPlane);
		planes.push_back(plane);
	}
}

void GPUFrustumCulling::BuildRootSignature(ID3D12Device* device)
{
	CD3DX12_ROOT_PARAMETER csSlotRootParameter[5];
	csSlotRootParameter[0].InitAsConstantBufferView(0); // cb for command
	csSlotRootParameter[1].InitAsShaderResourceView(0, 0); // srv for object transform
	csSlotRootParameter[2].InitAsShaderResourceView(0, 1); // srv for planes
	csSlotRootParameter[3].InitAsUnorderedAccessView(1); // uav for output and input
	csSlotRootParameter[4].InitAsUnorderedAccessView(0); // uav for visibility

	CD3DX12_ROOT_SIGNATURE_DESC csRootSigDesc(size(csSlotRootParameter), csSlotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&csRootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void GPUFrustumCulling::BuildComputeShader()
{
	mCSShaderByteCode = D3DUtil::CompileShader(L"Shaders\\GPUFrustumCulling.hlsl", nullptr, "CS", "cs_5_1");
}

void GPUFrustumCulling::BuildCommandSignature(ID3D12Device* device, ID3D12RootSignature* graphicsRootSignature)
{
	D3D12_INDIRECT_ARGUMENT_DESC argDescs[3] = {};
	argDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
	argDescs[0].VertexBuffer.Slot = 0;
	argDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
	argDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC commandSigDesc = {};
	commandSigDesc.pArgumentDescs = argDescs;
	commandSigDesc.NumArgumentDescs = size(argDescs);
	commandSigDesc.ByteStride = sizeof(IndirectCommand);

	ThrowIfFailed(device->CreateCommandSignature(&commandSigDesc, graphicsRootSignature, IID_PPV_ARGS(mCommandSignature.GetAddressOf())));
}

void GPUFrustumCulling::BuildIndirectCommandBuffer(ID3D12Device* device)
{
	auto sceneObjectCount = MaximumObjectAmountPerCommand;
	auto byteSize = CommandSizePerFrame;

	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mCommandAllocator.GetAddressOf())));

	mSceneObjectBuffer = make_unique<UploadBuffer<SceneObjectData>>(device, MaximumObjectAmount, false);
	mFrustumPlaneBuffer = make_unique<UploadBuffer<Plane>>(device, PlaneCount, false);
	mIndirectResetBuffer = make_unique<UploadBuffer<IndirectCommand>>(device, MaximumCommandAmount, false);
	// mIndirectBuffer = make_unique<UploadBuffer<IndirectCommand>>(device, MaximumCommandAmount, false);
	mCountBuffer = make_unique<CountCommand>();
	mCountBuffer->Count = sceneObjectCount;

	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uavDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(IndirectCommand) * MaximumCommandAmount,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uavDesc,
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		nullptr,
		IID_PPV_ARGS(mIndirectBuffer.GetAddressOf())));

	// Create Output Visibility
	auto visibilityBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		MaximumObjectAmount,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&visibilityBufferDesc,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&mIndirectOutputVisibilityBuffer)));
}

void GPUFrustumCulling::BuildPSO(ID3D12Device* device)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.pRootSignature = mRootSignature.Get();
	computePsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(mCSShaderByteCode->GetBufferPointer()),
		mCSShaderByteCode->GetBufferSize()
	};
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->CreateComputePipelineState(
		&computePsoDesc,
		IID_PPV_ARGS(&mPSO)));
}
