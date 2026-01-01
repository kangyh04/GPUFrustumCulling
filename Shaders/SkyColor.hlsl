float3 CalcSkyColor(float time, float y)
{
    // Map time [0,1] to t [0,4]
    int currColorIndex = time * 4;
    int nextColorIndex = (currColorIndex + 1) % 6;

    float3 topColor[] = {
        float3(0.05f, 0.05f, 0.3f),   // Night
        float3(1.0f, 0.5f, 0.0f),     // Sunrise
        float3(0.2f, 0.6f, 1.0f),     // Day
        float3(1.0f, 0.5f, 0.0f),     // Sunset
        float3(0.05f, 0.05f, 0.3f)    // Night
    };

    float3 bottomColor[] = {
        float3(0.0f, 0.0f, 0.05f),    // Night
        float3(1.0f, 0.7f, 0.4f),     // Sunrise
        float3(0.5f, 0.7f, 1.0f),     // Day
        float3(1.0f, 0.7f, 0.4f),     // Sunset
        float3(0.0f, 0.0f, 0.05f)     // Night
    };

    float3 top = lerp(topColor[currColorIndex], topColor[nextColorIndex], frac(time * 4));
    float3 bottom = lerp(bottomColor[currColorIndex], bottomColor[nextColorIndex], frac(time * 4));

    // top = lerp(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f), frac(time * 4));
    // bottom = lerp(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f), frac(time * 4));
    // bottom = bottomColor[currColorIndex];

    float gradient = pow(max(y, 0.0f), 0.5f);
    
    return lerp(top, bottom, gradient);
}