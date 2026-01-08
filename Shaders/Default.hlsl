#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "Common.hlsl"

struct ObjectData
{
    float4x4 World;
    float4x4 TexTransform;
    uint MaterialIndex;
    uint ObjPad0;
    uint ObjPad1;
    uint ObjPad2;
    float4 ObjPad3;
};

StructuredBuffer<ObjectData> gObjectData : register(t0, space2);
StructuredBuffer<uint> gVisibilityData : register(t0, space3);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD0;

    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    uint visibleId = gVisibilityData[instanceID];
    VertexOut vout = (VertexOut) 0.0f;

    ObjectData objData = gObjectData[visibleId];
    float4x4 world = objData.World;
    float4x4 texTransform = objData.TexTransform;
    uint matIndex = objData.MaterialIndex;

    vout.MatIndex = matIndex;

    MaterialData matData = gMaterialData[matIndex];
    
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;
    
    vout.NormalW = mul(vin.NormalL, (float3x3) world);
    
    vout.PosH = mul(posW, gViewProj);
    
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[pin.MatIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);

    pin.NormalW = normalize(pin.NormalW);

    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
    pin.NormalW, toEyeW, shadowFactor);
    
    float4 litColor = ambient + directLight;
    
    float3 r = reflect(-toEyeW, pin.NormalW);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, pin.NormalW, r);
    litColor.rgb += shininess * fresnelFactor;

    litColor.a = diffuseAlbedo.a;
    
    return litColor;
}