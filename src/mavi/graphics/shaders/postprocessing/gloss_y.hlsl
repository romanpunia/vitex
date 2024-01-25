#include "internal/layouts_shape.hlsl"
#include "internal/channels_effect.hlsl"
#include "internal/utils_random.hlsl"
#include "internal/utils_material.hlsl"

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
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
    float2 UV = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(UV);
	Material Mat = Materials[Frag.Material];
	float3 Normal = GetNormal(UV);
	float Roughness = GetRoughness(Frag, Mat);
	float Power = Roughness < Deadzone ? 0.0 : Roughness;
	float Force = Blur * Power;
	float3 Blurring = float3(0, 0, 0);
	float Count = max(1.0, Samples * Power);
	float Iterations = 0.0;

	[loop] for (float i = 0; i < Count; i++)
	{
		float2 T = UV + float2(0, Gaussian[i].x) * Texel * Force;
		[branch] if (dot(GetNormal(T), Normal) < Cutoff)
			continue;

		Blurring += Image.SampleLevel(Sampler, T, 0).xyz;
		Iterations++;
	}

	return float4(Blurring / max(1.0, Iterations), 1.0);
};