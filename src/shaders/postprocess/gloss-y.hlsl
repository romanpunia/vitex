#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/random.hlsl"
#include "std/core/material.hlsl"

cbuffer RenderConstant : register(b3)
{
	float Padding;
	float Deadzone;
	float Mips;
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
	float3 Normal = GetNormal(V.TexCoord.xy);
	float Roughness = GetRoughness(Frag, Mat);
	float Power = Roughness < Deadzone ? 0.0 : Roughness;
	float Force = Blur * Power;
	float3 Blurring = float3(0, 0, 0);
	float Count = max(1.0, Samples * Power);
	float Iterations = 0.0;

	[loop] for (float i = 0; i < Count; i++)
	{
		float2 TexCoord = V.TexCoord.xy + float2(0, FiboDisk[i].x) * Texel * Force;
		[branch] if (dot(GetNormal(TexCoord), Normal) < Cutoff)
			continue;

		Blurring += Image.SampleLevel(Sampler, TexCoord, 0).xyz;
		Iterations++;
	}

	return float4(Blurring / max(1.0, Iterations), 1.0);
};