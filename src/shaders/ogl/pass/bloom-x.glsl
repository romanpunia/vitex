#include "standard/space-uv"

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
	[loop] for (int x = -Samples; x < Samples; x++)
	{
        float2 Offset = float2(x, 0) * Texel * Scale;
        B += saturate(GetDiffuseLevel(V.TexCoord.xy + Offset, 0).xyz - Threshold);
	}

    B /= 2 * Samples;
	return float4(B * Intensity, 1.0);
};