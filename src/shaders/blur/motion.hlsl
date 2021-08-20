#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/sampler.hlsl"
#include "std/core/random.hlsl"

cbuffer RenderConstant : register(b3)
{
    float Samples;
    float Blur;
    float Motion;
    float Padding;
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
    float2 TexCoord = V.TexCoord.xy;
    float2 Velocity = GetSample(Image, TexCoord).xy * Motion;
    float3 Result = GetDiffuse(TexCoord, 0).xyz;
    TexCoord += Velocity;

    [loop] for (float i = 1.0; i < Samples; ++i)
    {
        float2 T = TexCoord + Velocity * FiboDisk[i] * Blur;
        Result += GetDiffuse(T, 0).xyz;
    }

    return float4(Result / Samples, 1.0); 
};