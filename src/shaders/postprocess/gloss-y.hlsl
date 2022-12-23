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
	Fragment Frag = GetFragment(V.TexCoord.xy);
	Material Mat = Materials[Frag.Material];
	float3 C = GetDiffuse(V.TexCoord.xy, 0).xyz;
	float3 N = GetNormal(V.TexCoord.xy);
	float3 B = float3(0, 0, 0);
	float R = GetRoughnessMip(Frag, Mat, 1.0);
	float G = Samples * R;
	float I = 0.0;

	[loop] for (int i = 0; i < G; i++)
	{
		float2 T = V.TexCoord.xy + float2(0, FiboDisk[i].y) * Texel * Blur * R;
		[branch] if (dot(GetNormal(T), N) < Cutoff)
			continue;

		B += Image.SampleLevel(Sampler, T, 0).xyz; I++;
	}

	return float4(B / max(1.0, I), 1.0);
};