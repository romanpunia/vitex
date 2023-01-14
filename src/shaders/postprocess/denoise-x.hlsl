#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/random.hlsl"
#include "std/core/material.hlsl"

cbuffer RenderConstant : register(b3)
{
	float3 Padding;
	float Cutoff;
	float2 Texel;
	float Samples;
	float Blur;
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
	float3 N = GetNormal(V.TexCoord.xy);
	float3 B = float3(0, 0, 0);
	float I = 0.0;

	[loop] for (int i = 0; i < Samples; i++)
	{
		float2 T = V.TexCoord.xy + float2(FiboDisk[i].x, 0) * Texel * Blur;
		[branch] if (dot(GetNormal(T), N) < 0.9)
			continue;

		B += Image.SampleLevel(Sampler, T, 0).xyz; I++;
	}

	return float4(B / max(1.0, I), 1.0);
};