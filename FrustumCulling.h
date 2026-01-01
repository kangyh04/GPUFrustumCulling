#pragma once

#include "Camera.h"
#include "RenderItem.h"
#include "FrameResource.h"

class FrustumCulling
{
public:
	void UpdateCameraFrustum(const Camera& camera);
	void CullRenderItems(const Camera& camera, const RenderItem* ritem, vector<ObjectData>& visibleRitems);
	void SetFrustumCullingEnabled(bool enabled) { mFrustumCullingEnabled = enabled; }

private:
	BoundingFrustum mCameraFrustum;
	bool mFrustumCullingEnabled = true;
};
