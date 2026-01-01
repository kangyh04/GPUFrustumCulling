#include "FrustumCulling.h"

void FrustumCulling::UpdateCameraFrustum(const Camera& camera)
{
	BoundingFrustum::CreateFromMatrix(mCameraFrustum, camera.GetProj());
}

void FrustumCulling::CullRenderItems(const Camera& camera, const RenderItem* ritem, vector<ObjectData>& visibleRitems)
{
	XMMATRIX view = camera.GetView();
	auto detView = XMMatrixDeterminant(view);
	XMMATRIX invView = XMMatrixInverse(&detView, view);

	const auto& instanceData = ritem->Instances;

	for (UINT i = 0; i < (UINT)instanceData.size(); ++i)
	{
		XMMATRIX world = XMLoadFloat4x4(&instanceData[i].World);
		XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].TexTransform);

		auto detWorld = XMMatrixDeterminant(world);
		XMMATRIX invWorld = XMMatrixInverse(&detWorld, world);

		XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

		BoundingFrustum localSpaceFrustum;
		mCameraFrustum.Transform(localSpaceFrustum, viewToLocal);

		if ((localSpaceFrustum.Contains(ritem->Bounds) != DISJOINT) || !mFrustumCullingEnabled)
		{
			ObjectData data;
			XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));
			visibleRitems.push_back(data);
		}
	}
}
