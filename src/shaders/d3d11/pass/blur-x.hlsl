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
    float3 B = float3(0, 0, 0);
	[loop] for (float x = -Samples; x < Samples; x++)
	{
        float2 T = V.TexCoord.xy + float2(x, 0) * Texel * Blur;
        B += GetSampleLevel(Image, T, 0).xyz;
	}

    return float4(B * Power / (2.0 * Samples), 1.0);
};