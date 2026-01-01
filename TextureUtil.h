#pragma once

#include "D3DUtil.h"
#include "DDSTextureLoader.h"

class TextureUtil
{
public:
	static unique_ptr<Texture> LoadTexture(
		ID3D12Device* d3dDevice,
		ID3D12GraphicsCommandList* cmdList,
		string name)
	{
		auto tex = make_unique<Texture>();
		tex->Name = name;
		// tex->Filename = L"Textures/" + AnsiToWString(name) + L".dds";
		tex->Filename = RelativePath() + AnsiToWString(name) + L".dds";

		ThrowIfFailed(CreateDDSTextureFromFile12(
			d3dDevice,
			cmdList,
			tex->Filename.c_str(),
			tex->Resource,
			tex->UploadHeap));

		return tex;
	}

private:
	static wstring RelativePath() { return  L"../../Textures/"; }
};