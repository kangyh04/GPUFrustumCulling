#define threadBlockSize 128

struct SceneObjectData
{
    float4 posW;
    float3 size;
    uint pad0;
};

struct IndirectCommand
{
    float4 vbv;
    float4 ibv;
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
    uint pad0;
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
StructuredBuffer<Plane> gFrustumPlanes : register(t0, space1);
RWStructuredBuffer<IndirectCommand> gCullingOutputs : register(u0);
// RWByteAddressBuffer<IndirectCommand> gCullingOutputs : register(u0);
RWStructuredBuffer<uint> gVisibilityOutputs : register(u1);


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
void CS(uint3 groupId : SV_GroupID, uint3 DTid : SV_DispatchThreadID)
{
    // NOTE : Depends on groupId, Indirect Command'd be changed
    uint commandIndex = groupId.x;
    uint index = groupId.x * threadBlockSize + DTid.x;
    if (commandIndex < gCommandCount)
    {
        SceneObjectData objData = gObjectData[index];
        bool isVisible = IsBoxInFrustum(objData.posW, objData.size);
        if (isVisible)
        {
            uint visibilityIndex;
            InterlockedAdd(gCullingOutputs[commandIndex].InstanceCount, 1, visibilityIndex);
            visibilityIndex += threadBlockSize * groupId.x;
            gVisibilityOutputs[visibilityIndex] = index;
        }
    }
}
