#pragma once

#include "D3DUtil.h"
#include "FrameResource.h"
#include "UploadBuffer.h"

struct FrameWave
{
public:
	FrameWave(ID3D12Device* device, UINT waveVertCount);
	FrameWave(const FrameWave& rhs) = delete;
	FrameWave& operator= (const FrameWave& rhs) = delete;
	~FrameWave();

	unique_ptr<UploadBuffer<Vertex>> WavesVB = nullptr;

	UINT64 Fence = 0;
};

FrameWave::FrameWave(ID3D12Device* device, UINT waveVertCount)
{
	WavesVB = make_unique<UploadBuffer<Vertex>>(device, waveVertCount, false);
}

FrameWave::~FrameWave()
{

}
