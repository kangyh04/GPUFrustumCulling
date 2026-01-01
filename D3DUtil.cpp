#include "D3DUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;
using namespace std;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}

bool D3DUtil::IsKeyDown(int vKeyCode)
{
	return (GetAsyncKeyState(vKeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> D3DUtil::LoadBinary(const wstring& filename)
{
	ifstream fin(filename, ios::binary);

	fin.seekg(0, ios_base::end);
	ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, ios_base::beg);

	ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read((char*)blob->GetBufferPointer(), size);
	fin.close();

	return blob;
}

ComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(device->CreateCommittedResource(
		// &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		// &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(device->CreateCommittedResource(
		// &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		// &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		&uploadResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	CD3DX12_RESOURCE_BARRIER commonToCopyDestBarrier =
		CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
// 	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
// 		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	cmdList->ResourceBarrier(1, &commonToCopyDestBarrier);
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	CD3DX12_RESOURCE_BARRIER genericReadBarrier =
		CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
// 	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
// 		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	cmdList->ResourceBarrier(1, &genericReadBarrier);

	return defaultBuffer;
}

ComPtr<ID3DBlob> D3DUtil::CompileShader(
	const wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const string& entrypoint,
	const string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;

	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(),
		target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}

wstring DxException::ToString() const
{
	_com_error err(ErrorCode);
	wstring msg = err.ErrorMessage();

	return FunctionName + L"failed in " + Filename + L"; line " + to_wstring(LineNumber) + L"; error: " + msg;
}
