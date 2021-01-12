#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/sampler"
#include "sdk/core/random"

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

Texture2D Image : register(t5);

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    float3 B = float3(0, 0, 0);
	[loop] for (int i = 0; i < Samples; i++)
    {
        float2 Offset = float2(0, FiboDisk[i].y) * Texel * Scale;
        B += saturate(GetSampleLevel(Image, V.TexCoord.xy + Offset, 0).xyz - Threshold);
    }

    B /= Samples;
	return float4(GetDiffuse(V.TexCoord.xy, 0).xyz + B * Intensity, 1.0);
};