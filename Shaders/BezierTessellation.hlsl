#include "LightingUtil.hlsl"
#include "BezierUtil.hlsl"

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 cbPerObjectPad2;
    
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    vout.PosL = vin.PosL;

    return vout;
}

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    pt.EdgeTess[0] = 25;
    pt.EdgeTess[1] = 25;
    pt.EdgeTess[2] = 25;
    pt.EdgeTess[3] = 25;
    
    pt.InsideTess[0] = 25;
    pt.InsideTess[1] = 25;

    return pt;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
BezierHullOut HS(InputPatch<VertexOut, 16> p,
            uint i : SV_OutputControlPointID,
            uint patchId : SV_PrimitiveID)
{
    BezierHullOut hout = (BezierHullOut) 0.0f;
    hout.PosL = p[i].PosL;
    return hout;
}

struct DomainOut
{
    float4 PosH : SV_POSITION;
};

[domain("quad")]
DomainOut DS(PatchTess patchTess,
            float2 uv : SV_DomainLocation,
            const OutputPatch<BezierHullOut, 16> quad)
{
    DomainOut dout = (DomainOut) 0.0f;

    float4 basisU = BernsteinBasis(uv.x);
    float4 basisV = BernsteinBasis(uv.y);
    
    float3 p = CubicBezierSum(quad, basisU, basisV);

    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);

    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}