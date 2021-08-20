#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/sampler.hlsl"
#include "std/core/position.hlsl"

cbuffer RenderConstant : register(b3)
{
    matrix LastViewProjection;
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	float2 TexCoord = GetTexCoord(V.TexCoord);
    float3 Origin = GetPosition(TexCoord, GetDepth(TexCoord));
    float4 Offset = mul(float4(Origin, 1.0), LastViewProjection);
    float2 Delta = (V.TexCoord.xy - Offset.xy / Offset.w) / 2.0;

    return float4(Delta, 0.0, 1.0);
};