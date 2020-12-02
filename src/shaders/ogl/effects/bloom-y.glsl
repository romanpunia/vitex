#include "standard/space-uv"

Texture2D Image : register(t5);

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Intensity;
    float Threshold;
    float Scale;
    float Padding1;
    float Padding2;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float3 B = float3(0, 0, 0);
    [loop] for (int y = -Samples; y < Samples; y++)
    {
        float2 Offset = float2(0, y) * Texel * Scale;
        B += saturate(GetSampleLevel(Image, V.TexCoord.xy + Offset, 0).xyz - Threshold);
    }

    B /= 2 * Samples;
	return float4(GetDiffuse(V.TexCoord.xy).xyz + B * Intensity, 1.0);
};