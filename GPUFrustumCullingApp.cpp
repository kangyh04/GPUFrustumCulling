#include "BaseApp.h"
#include "DDSTextureLoader.h"
#include "MeshUtil.h"
#include "MaterialUtil.h"
#include "StaticSamplers.h"
#include "PSOUtil.h"

class GPUFrustumCullingApp : public BaseApp
{
public:
	GPUFrustumCullingApp(HINSTANCE hInstance);
	GPUFrustumCullingApp(const GPUFrustumCullingApp& rhs) = delete;
	GPUFrustumCullingApp& operator=(const GPUFrustumCullingApp& rhs) = delete;
	~GPUFrustumCullingApp();
protected:
	virtual void Build() override;

private:
	void LoadTextures();
	void LoadShapes();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSOs();
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		GPUFrustumCullingApp app(hInstance);
		if (!app.Initialize())
		{
			return 0;
		}
		return app.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

GPUFrustumCullingApp::GPUFrustumCullingApp(HINSTANCE hInstance)
	:BaseApp(hInstance)
{
}

GPUFrustumCullingApp::~GPUFrustumCullingApp()
{
	if (md3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

void GPUFrustumCullingApp::Build()
{
	LoadTextures();
	LoadShapes();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildPSOs();
}

void GPUFrustumCullingApp::LoadTextures()
{
	vector<string> texNames =
	{
		"defaultDiffuse",
	};

	vector<wstring> texFilenames =
	{
		L"Textures/white1x1.dds",
	};

	for (int i = 0; i < texNames.size(); ++i)
	{
		auto texMap = make_unique<Texture>();
		texMap->Name = texNames[i];
		texMap->Filename = texFilenames[i];
		ThrowIfFailed(
			CreateDDSTextureFromFile12(
				md3dDevice.Get(),
				mCommandList.Get(),
				texMap->Filename.c_str(),
				texMap->Resource,
				texMap->UploadHeap));
		mTextures[texMap->Name] = move(texMap);
	}
}

void GPUFrustumCullingApp::LoadShapes()
{
	vector<string> shapeNames =
	{
		"skull",
	};

	vector<wstring> shapeFilenames =
	{
		L"Models/skull.txt",
	};

	auto mesh = MeshUtil::LoadMesh(
		md3dDevice.Get(),
		mCommandList.Get(),
		shapeNames[0],
		shapeFilenames[0]);

	mGeometries[mesh->Name] = move(mesh);
}

void GPUFrustumCullingApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE objTable0;
	objTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[passCBRootParameterIndex].InitAsConstantBufferView(0);
	// slotRootParameter[1].InitAsConstantBufferView(1);
	// slotRootParameter[objRootParameterIndex].InitAsShaderResourceView(1, 1);
	slotRootParameter[objRootParameterIndex].InitAsDescriptorTable(1, &objTable0);
	slotRootParameter[matBufferRootParameterIndex].InitAsShaderResourceView(0, 1);
	slotRootParameter[texRootParameterIndex].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = StaticSampler::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));

	// CD3DX12_ROOT_PARAMETER csSlotRootParameter[5];
}

void GPUFrustumCullingApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = (UINT)mTextures.size() + gNumFrameResources;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&srvHeapDesc,
		IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (auto& tex : mTextures)
	{
		auto resource = tex.second->Resource;
		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(
			resource.Get(),
			&srvDesc,
			hDescriptor);
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = (UINT)mAllRitems[0]->Instances.size();
		srvDesc.Buffer.StructureByteStride = sizeof(ObjectData);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		auto instanceBuffer = mFrameResources[i]->ObjectCB->Resource();
		md3dDevice->CreateShaderResourceView(
			instanceBuffer,
			&srvDesc,
			hDescriptor);
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}
}

void GPUFrustumCullingApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = D3DUtil::CompileShader(
		L"Shaders\\Default.hlsl",
		nullptr,
		"VS",
		"vs_5_1");
	mShaders["opaquePS"] = D3DUtil::CompileShader(
		L"Shaders\\Default.hlsl",
		nullptr,
		"PS",
		"ps_5_1");
// 	mShaders["cullCS"] = D3DUtil::CompileShader(
// 		L"Shaders/GPUFrustumCulling.hlsl",
// 		nullptr,
// 		"CS",
// 		"cs_5_0");

	mStdInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void GPUFrustumCullingApp::BuildMaterials()
{
	vector<string> matNames =
	{
		"defaultMat",
	};

	vector<wstring> matFilenames =
	{
		L"Materials/defaultMat.txt",
	};

	auto defaultMat = MaterialUtil::LoadMaterial(
		0,
		0,
		matNames[0],
		matFilenames[0]);

	mMaterials[defaultMat->Name] = move(defaultMat);
}

void GPUFrustumCullingApp::BuildRenderItems()
{
	auto skullRitem = make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 0;
	skullRitem->Mat = mMaterials["defaultMat"].get();
	skullRitem->Geo = mGeometries["skull"].get();
	skullRitem->PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;

	constexpr int n = 5;
	auto instanceCount = n * n * n;
	skullRitem->Instances.resize(instanceCount);

	constexpr float width = 200.0f;
	constexpr float height = 200.0f;
	constexpr float depth = 200.0f;

	float x = -0.5f * width;
	float y = -0.5f * height;
	float z = -0.5f * depth;
	float dx = width / (n - 1);
	float dy = height / (n - 1);
	float dz = depth / (n - 1);

	for (int k = 0; k < n; ++k)
	{
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				int index = k * n * n + i * n + j;
				skullRitem->Instances[index].World = XMFLOAT4X4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					x + j * dx, y + i * dy, z + k * dz, 1.0f
				);

				skullRitem->Instances[index].TexTransform = MathHelper::Identity4x4();
				skullRitem->Instances[index].MaterialIndex = 0;
			}
		}
	}

	mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());

	mAllRitems.push_back(move(skullRitem));
}

void GPUFrustumCullingApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(make_unique<FrameResource>(
			md3dDevice.Get(),
			1,
			(UINT)mAllRitems[0]->Instances.size(),
			(UINT)mMaterials.size()));
	}
}

void GPUFrustumCullingApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
		&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
// 	auto opaquePsoDesc = PSOUtil::MakeOpaquePSODesc(
// 		mShaders["standardVS"].Get(),
// 		mShaders["opaquePS"].Get(),
// 		mStdInputLayout,
// 		mRootSignature.Get(),
// 		mBackBufferFormat,
// 		mDepthStencilFormat,
// 		m4xMsaaState,
// 		m4xMsaaQuality);
// 
// 	mPsoDescs["opaque"] = opaquePsoDesc;
// 
// 	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(
// 		&opaquePsoDesc,
// 		IID_PPV_ARGS(&mPSOs["opaque"])));
}
