#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/random.hlsl"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
	float Mips;
	float Time;
}

Texture2D LUT : register(t6);

float GetAvg(float2 TexCoord)
{
	float3 Color = GetDiffuseSample(TexCoord).xyz;
	return (Color.x + Color.y + Color.z) / 3.0;
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float ps_main(VOutput V) : SV_TARGET0
{
	float Result = 0.0;
	[unroll] for (float i = 0; i < 16.0; i++)
		Result += GetAvg(V.TexCoord.xy + FiboDisk[i] * Texel);
	
	float Subresult = LUT.Sample(Sampler, V.TexCoord.xy).r;
	return Subresult + (Result / 4.0 - Subresult) * (1.0 - exp(-Time));
};