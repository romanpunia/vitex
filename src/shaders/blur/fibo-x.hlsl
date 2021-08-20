#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/sampler.hlsl"
#include "std/core/random.hlsl"

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Blur;
    float3 Padding;
    float Power;
}

Texture2D Image : register(t5);

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{    
    float3 B = float3(0, 0, 0);
	[loop] for (int i = 0; i < Samples; i++)
	{
        float2 T = V.TexCoord.xy + float2(FiboDisk[i].x, 0) * Texel * Blur;
        B += GetSampleLevel(Image, T, 0).xyz;
	}

    return float4(B * Power / Samples, 1.0);
};