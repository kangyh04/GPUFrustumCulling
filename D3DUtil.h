#pragma once

#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "D3DX12.h"
#include "MathHelper.h"

extern const int gNumFrameResources;

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

inline void D3DSetDebugName(IDXGIObject* obj, const char* name)
{
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline void D3DSetDebugName(ID3D12Device* obj, const char* name)
{
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline void D3DSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class D3DUtil
{
public:
	static bool IsKeyDown(int vKeyCode);

	static std::string ToString(HRESULT hr);

	static UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		return (byteSize + 255) & ~255;
	}

	static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defins,
		const std::string& entrypoint,
		const std::string& target);
};

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineeNumber);

	std::wstring ToString() const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
	std::string Name;

	ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	unordered_map<string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

struct Light
{
	XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;
	XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
	float FalloffEnd = 10.0f;
	XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
	float SpotPower = 64.0f;
};

#define MaxLights 16

struct MaterialConstants
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25;

	XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Instance
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex = 0;
};

struct Material
{
	string Name;

	int MatCBIndex = -1;

	int DiffuseSrvHeapIndex = -1;

	int NormalSrvHeapIndex = -1;

	int NumFramesDirty = gNumFrameResources;

	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Texture
{
	string Name;
	wstring Filename;

	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

inline void CheckD3D12Message(ID3D12Device* device, const char* context)
{
#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		UINT64 messageCount = infoQueue->GetNumStoredMessages();

		for (UINT64 i = 0; i < messageCount; ++i)
		{
			SIZE_T messageLength = 0;
			infoQueue->GetMessageW(i, nullptr, &messageLength);

			auto message = make_unique<BYTE[]>(messageLength);
			auto pMessage = reinterpret_cast<D3D12_MESSAGE*>(message.get());

			infoQueue->GetMessageW(i, pMessage, &messageLength);

			printf("[%s] D3D12 Message: %s\n", context, pMessage->pDescription);
			OutputDebugStringA(pMessage->pDescription);
			OutputDebugStringA("\n");
		}

		infoQueue->ClearStoredMessages();
	}
#endif
}

inline wstring ToHexString(HRESULT hr)
{
	wchar_t buffer[16];
	swprintf_s(buffer, L"%08X", static_cast<unsigned int>(hr));
	return wstring(buffer);
}

inline wstring GetDetailedErrorMessage(HRESULT hr, const wstring& functionCall)
{
	wstring message = L"HRESULT: 0x" + ToHexString(hr) + L"\n";
	message += L"Function: " + functionCall + L"\n";

	LPWSTR messageBuffer = nullptr;
	DWORD size = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

	if (size && messageBuffer)
	{
		message += L"Message: " + wstring(messageBuffer, size) + L"\n";
		LocalFree(messageBuffer);
	}

	return message;
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
	HRESULT hr__ = (x);                                              \
	wstring wfn = AnsiToWString(__FILE__);							\
	if(FAILED(hr__)) {                                               \
		wstring errorMsg = GetDetailedErrorMessage(hr__, L#x);              \
		throw DxException(hr__, errorMsg, wfn.c_str(), __LINE__);          \
	}                                                                 \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) {if(x){x->Release(); x = 0; } }
#endif
