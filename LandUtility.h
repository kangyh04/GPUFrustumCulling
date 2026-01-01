#pragma once
#include "D3DUtil.h"

using namespace DirectX;

class LandUtility
{
public:
	static float GetHillsHeight(float x, float z)
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}

	static XMFLOAT3 GetHillsNormal(float x, float z)
	{
		XMFLOAT3 n(
			-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
			1.0f,
			-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

		XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
		XMStoreFloat3(&n, unitNormal);
		return n;
	}
};
