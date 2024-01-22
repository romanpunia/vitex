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
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
    float2 UV = GetTexCoord(V.TexCoord);
	float3 N = GetNormal(UV);
	float3 B = float3(0, 0, 0);
	float I = 0.0;

	[loop] for (int i = 0; i < Samples; i++)
	{
		float2 T = UV + float2(0, Gaussian[i].y) * Texel * Blur;
		[branch] if (dot(GetNormal(T), N) < Cutoff)
			continue;

		B += Image.SampleLevel(Sampler, T, 0).xyz; I++;
	}

	return float4(B / max(1.0, I), 1.0);
};