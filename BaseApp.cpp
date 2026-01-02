#include "BaseApp.h"
#include "Input.h"
#include <iostream>
#include <cmath>

const int gNumFrameResources = 3;

BaseApp::BaseApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{

}

BaseApp::~BaseApp()
{

}

bool BaseApp::Initialize()
{
#if defined(DEBUG) | defined(_DEBUG)
	D3DApp::CreateDebugConsole();
	EnableD3D12DebugLayer();
#endif
	if (!D3DApp::Initialize())
	{
		return false;
	}

	if (!Input::GetInstance().Initialize(mhMainWnd))
	{
		return false;
	}

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	ThrowIfFailed(mComputeCommandList->Reset(mComputeCmdListAlloc.Get(), nullptr));

	Build();

	// BuildWireFramePSOs();

	ThrowIfFailed(mCommandList->Close());

	ThrowIfFailed(mComputeCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ID3D12CommandList* computeCmdsLists[] = { mComputeCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(computeCmdsLists), computeCmdsLists);

	// FlushCommandQueue();
	FlushAllCommandQueues();

	return true;
}

void BaseApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void BaseApp::Update(const Timer& gt)
{
	OnKeyboardInput(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateInstanceBuffer(gt);
	UpdateMaterialBuffer(gt);
	UpdateMainPassCB(gt);
}

void BaseApp::DoComputeWork(const Timer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->ComputeCmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mComputeCommandList->Reset(cmdListAlloc.Get(), nullptr));

	// Set compute root signature
	CullRenderItems();

	ThrowIfFailed(mComputeCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mComputeCommandList.Get() };
	mComputeCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	mCurrFrameResource->ComputeFence = ++mCurrentComputeFence;

	mComputeCommandQueue->Signal(mComputeFence.Get(), mCurrentComputeFence);
}

void BaseApp::Draw(const Timer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	ID3D12DescriptorHeap* descriptorheaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorheaps), descriptorheaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(matBufferRootParameterIndex, matBuffer->GetGPUVirtualAddress());

	mCommandList->SetGraphicsRootDescriptorTable(texRootParameterIndex, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	mCommandList->ResourceBarrier(1, &toRenderTarget);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	auto currentBackBufferView = CurrentBackBufferView();
	auto depthStencilView = DepthStencilView();
	mCommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &depthStencilView);

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(passCBRootParameterIndex, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &toPresent);

	ThrowIfFailed(mCommandList->Close());

	mCommandQueue->Wait(mComputeFence.Get(), mCurrentComputeFence);

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrFrameResource->Fence = ++mCurrentFence;

	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

}

void BaseApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void BaseApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BaseApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BaseApp::OnKeyInputed(LPARAM lParam)
{
	// Input::GetInstance().ProcessInput(lParam);
}

void BaseApp::OnKeyboardInput(const Timer& gt)
{
	const float dt = gt.GetDeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
	{
		mCamera.Walk(10.0f * dt);
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		mCamera.Walk(-10.0f * dt);
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		mCamera.Strafe(-10.0f * dt);
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		mCamera.Strafe(10.0f * dt);
	}

	mCamera.UpdateViewMatrix();
}

void BaseApp::CullRenderItems()
{
}

void BaseApp::UpdateInstanceBuffer(const Timer& gt)
{
	auto currInstanceBuffer = mCurrFrameResource->ObjectCB.get();

	for (auto& e : mAllRitems)
	{
		UINT objCount = (UINT)e->Instances.size();

		for (int i = 0; i < objCount; ++i)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->Instances[i].World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->Instances[i].TexTransform);

			ObjectData objData;
			XMStoreFloat4x4(&objData.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objData.TexTransform, XMMatrixTranspose(texTransform));
			objData.MaterialIndex = e->Mat->MatCBIndex;

			currInstanceBuffer->CopyData(e->ObjCBIndex + i, objData);
		}

		e->InstanceCount = objCount;
	}
}

void BaseApp::UpdateMaterialBuffer(const Timer& gt)
{
	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		auto mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

			mat->NumFramesDirty--;
		}
	}
}

void BaseApp::UpdateMainPassCB(const Timer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	auto detView = XMMatrixDeterminant(view);
	auto detProj = XMMatrixDeterminant(proj);
	auto detViewProj = XMMatrixDeterminant(viewProj);
	XMMATRIX invView = XMMatrixInverse(&detView, view);
	XMMATRIX invProj = XMMatrixInverse(&detProj, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&detViewProj, viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.GetTotalTime();
	mMainPassCB.DeltaTime = gt.GetDeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void BaseApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const vector<RenderItem*>& ritems)
{
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	// cmdList->SetGraphicsRootShaderResourceView(0, objectCB->GetGPUVirtualAddress());
	auto heap = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	UINT texOffset = (UINT)mTextures.size();
	UINT offset = texOffset + mCurrFrameResourceIndex;
	CD3DX12_GPU_DESCRIPTOR_HANDLE objHandle(heap, offset, mCbvSrvDescriptorSize);

	cmdList->SetGraphicsRootDescriptorTable(objRootParameterIndex, objHandle);

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		auto vertexBufferView = ri->Geo->VertexBufferView();
		auto indexBufferView = ri->Geo->IndexBufferView();
		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		cmdList->IASetIndexBuffer(&indexBufferView);
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		cmdList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void BaseApp::BuildWireFramePSOs()
{
	for (auto& desc : mPsoDescs)
	{
		auto psoDesc = desc.second;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		ComPtr<ID3D12PipelineState> wireframePSO;
		ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&wireframePSO)));
		mPSOs[desc.first + "_wireframe"] = wireframePSO;
	}
}

void BaseApp::EnableD3D12DebugLayer()
{
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();

		ComPtr<ID3D12Debug1> debugController1;
		if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
		{
			debugController1->SetEnableGPUBasedValidation(true);
		}
	}
}
