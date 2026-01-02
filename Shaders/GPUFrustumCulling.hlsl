#define threadBlockSize 128

struct SceneObjectData
{
    float4 posW;
    float3 size;
    uint pad2;
};

struct IndirectCommand
{
    uint instanceId;
    uint pad0;
    uint pad1;
    uint pad2;
};

struct Plane
{
    float3 normal;
    float distance;
};

cbuffer cbRoot : register(b0)
{
    uint gCommandCount;
    uint pad0;
    uint pad1;
    uint pad2;
}

StructuredBuffer<SceneObjectData> gObjectData : register(t0, space0);
StructuredBuffer<IndirectCommand> gCullingInputs : register(t0, space1);
StructuredBuffer<Plane> gFrustumPlanes : register(t0, space2);
AppendStructuredBuffer<IndirectCommand> gCullingOutputs : register(u0);

bool IsBoxInFrustum(float4 posW, float3 size)
{
    float3 halfSize = size * 0.5;

    for (int i = 0; i < 6; ++i)
    {
        Plane plane = gFrustumPlanes[i];
        float3 signedHalfSize = (plane.normal > 0) ? halfSize : -halfSize;
        float3 boxVertex = posW.xyz + signedHalfSize;
        if (dot(plane.normal, boxVertex) + plane.distance < 0)
        {
            return false;
        }
    }
    return true;
}

[numthreads(threadBlockSize, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    uint index = DTid.x;
    uint instanceId = gCullingInputs[index].instanceId;
    if (instanceId < gCommandCount)
    {
        SceneObjectData objData = gObjectData[instanceId];
        bool isVisible = IsBoxInFrustum(objData.posW, objData.size);
        if (isVisible)
        {
            IndirectCommand inputCmd = gCullingInputs[instanceId];
            gCullingOutputs.Append(inputCmd);
        }
    }
}
