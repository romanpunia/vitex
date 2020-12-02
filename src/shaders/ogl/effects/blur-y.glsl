#include "standard/space-uv"

Texture2D Image : register(t5);

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Blur;
    float Power;
    float Additive;
    float2 Padding;
}

float4 PS(VertexResult V) : SV_TARGET0
{    
    float3 C = GetDiffuse(V.TexCoord.xy).xyz;
    float3 B = float3(0, 0, 0);

    [loop] for (float y = -Samples; y < Samples; y++)
    {
        float2 T = V.TexCoord.xy + float2(0, y) * Texel * Blur;
        B += GetSampleLevel(Image, T, 0).xyz;
    }

    float3 R = B * Power / (2.0 * Samples);
    return float4(Additive > 0.0 ? C + R : C * R, 1.0);
};