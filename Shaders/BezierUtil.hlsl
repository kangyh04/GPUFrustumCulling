struct BezierHullOut
{
    float3 PosL : POSITION;
};

float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t);
}

float3 CubicBezierSum(const OutputPatch<BezierHullOut, 16> bezpatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum = basisV.x * (basisU.x * bezpatch[0].PosL + basisU.y * bezpatch[1].PosL + basisU.z * bezpatch[2].PosL + basisU.w * bezpatch[3].PosL);
    sum += basisV.y * (basisU.x * bezpatch[4].PosL + basisU.y * bezpatch[5].PosL + basisU.z * bezpatch[6].PosL + basisU.w * bezpatch[7].PosL);
    sum += basisV.z * (basisU.x * bezpatch[8].PosL + basisU.y * bezpatch[9].PosL + basisU.z * bezpatch[10].PosL + basisU.w * bezpatch[11].PosL);
    sum += basisV.w * (basisU.x * bezpatch[12].PosL + basisU.y * bezpatch[13].PosL + basisU.z * bezpatch[14].PosL + basisU.w * bezpatch[15].PosL);

    return sum;
}

float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(-3 * invT * invT,
                   3 * invT * invT - 6 * t * invT,
                   6 * t * invT - 3 * t * t,
                   3 * t * t);
}