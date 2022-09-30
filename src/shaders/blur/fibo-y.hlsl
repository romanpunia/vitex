#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/random.hlsl"

cbuffer RenderConstant : register(b3)
{
	float3 Padding;
	float Power;
	float2 Texel;
	float Samples;
	float Blur;
}

Texture2D Image : register(t5);

float GetAntibleed(float TexCoord)
{
#ifdef TARGET_D3D
	return smoothstep(0.0, 0.05, TexCoord) * (1.0 - smoothstep(0.95, 1.0, TexCoord));
#else
	return smoothstep(0.0, 0.05, TexCoord) * smoothstep(0.95, 1.0, TexCoord);
#endif
}

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
		float2 T = V.TexCoord.xy + float2(0, FiboDisk[i].y) * Texel * Blur;
		B += Image.SampleLevel(Sampler, T, 0).xyz;
	}
#ifdef TARGET_D3D
	float A = GetAntibleed(-V.TexCoord.y);
#else
	float A = GetAntibleed(V.TexCoord.y);
#endif
	return float4(B * A * Power / Samples, 1.0);
};