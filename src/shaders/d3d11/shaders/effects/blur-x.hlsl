#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/sampler"
#include "sdk/core/random"

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Blur;
    float Power;
    float Additive;
    float2 Padding;
}

Texture2D Image : register(t5);

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = V.Position;
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{    
    float3 B = float3(0, 0, 0);
	[loop] for (int i = 0; i < Samples; i++)
	{
        float2 T = V.TexCoord.xy + float2(FiboDisk[i].x, 0) * Texel * Blur;
        B += GetSampleLevel(Image, T, 0).xyz;
	}

    return float4(B * Power / Samples, 1.0);
};