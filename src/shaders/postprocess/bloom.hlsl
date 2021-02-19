#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/sampler"

cbuffer RenderConstant : register(b3)
{
    float2 Padding;
    float Intensity;
    float Threshold;
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
    float3 Color = GetDiffuse(V.TexCoord.xy, 0).xyz;
	return float4(max(0.0, Color - Threshold) * Intensity, 1.0);
};