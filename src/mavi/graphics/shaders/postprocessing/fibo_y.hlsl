#include "internal/layouts_shape.hlsl"
#include "internal/channels_effect.hlsl"
#include "internal/utils_random.hlsl"

cbuffer RenderConstant : register(b3)
{
	float3 Padding;
	float Power;
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
	float3 B = float3(0, 0, 0);

	[loop] for (int i = 0; i < Samples; i++)
	{
		float2 T = UV + float2(0, Gaussian[i].y) * Texel * Blur;
		B += Image.SampleLevel(Sampler, T, 0).xyz;
	}
	
	return float4(B * Power / Samples, 1.0);
};