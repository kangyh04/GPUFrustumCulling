#include "GPUFrustumCulling.h"

void GPUFrustumCulling::BuildResources(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, vector<IndirectCommand> sceneObjectCommand)
{
	auto sceneObjectCount = static_cast<UINT>(sceneObjectCommand.size());
	auto byteSize = sizeof(IndirectCommand) * sceneObjectCount;

	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mCommandAllocator.GetAddressOf())));

	mSceneObjectBuffer = make_unique<UploadBuffer<SceneObjectData>>(device, sceneObjectCount, false);
	mFrustumPlaneBuffer = make_unique<UploadBuffer<Plane>>(device, PlaneCount, false);
	mCountBuffer = make_unique<UploadBuffer<CountCommand>>(device, 1, true);

	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	auto indirectInputBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&indirectInputBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mIndirectInputCommandBuffer)));

	auto updateHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&updateHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mIndirectInputUploadBuffer)));

	void* data = nullptr;
	ThrowIfFailed(mIndirectInputUploadBuffer->Map(0, nullptr, &data));
	memcpy(data, sceneObjectCommand.data(), byteSize);
	mIndirectInputUploadBuffer->Unmap(0, nullptr);

	auto toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectInputCommandBuffer.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->ResourceBarrier(1, &toCopyDest);

	commandList->CopyBufferRegion(
		mIndirectInputCommandBuffer.Get(),
		0,
		mIndirectInputUploadBuffer.Get(),
		0,
		byteSize);

	auto toGenericRead = CD3DX12_RESOURCE_BARRIER::Transition(
		mIndirectInputCommandBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	commandList->ResourceBarrier(1, &toGenericRead);

	auto indirectOutputBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		byteSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&indirectOutputBufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mIndirectOutputCommandBuffer)));
}

void GPUFrustumCulling::UpdateSceneObjectBuffer(ID3D12Device* device, const vector<SceneObjectData>& sceneObjects)
{
	for (UINT i = 0; i < sceneObjects.size(); ++i)
	{
		mSceneObjectBuffer->CopyData(i, sceneObjects[i]);
	}
}

void GPUFrustumCulling::UpdateFrustumPlaneBuffer(ID3D12Device* device, const XMMATRIX viewProj)
{
	vector<Plane> frustumPlanes;
	ExtractPlanes(viewProj, frustumPlanes);
	for (UINT i = 0; i < frustumPlanes.size(); ++i)
	{
		mFrustumPlaneBuffer->CopyData(i, frustumPlanes[i]);
	}
}

void GPUFrustumCulling::CullSceneObjects(ID3D12GraphicsCommandList* cmdList, UINT sceneObjectCount, ID3D12Resource* indirectInputCommandBuffer, ID3D12Resource* indirectOutputCommandBuffer)
{
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
