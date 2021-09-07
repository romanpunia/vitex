#include "std/layouts/shape.hlsl"
#include "std/channels/geometry.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/core/sampler.hlsl"

cbuffer RenderConstant : register(b3)
{
	matrix DecalViewProjection;
};

Texture2D LDepthBuffer : register(t8);

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), ob_WorldViewProj);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	float2 Coord = float2(0.5 + 0.5 * V.TexCoord.x / V.TexCoord.w, 0.5 - 0.5 * V.TexCoord.y / V.TexCoord.w);
	float4 Position = mul(float4(Coord.x * 2.0 - 1.0, 1.0 - Coord.y * 2.0, GetSampleLevel(LDepthBuffer, Coord, 0).x, 1.0), vb_InvViewProj);
	float4 Projected = mul(Position / Position.w, DecalViewProjection);

	Coord = float2(Projected.x / Projected.w / 2.0 + 0.5, 1 - (Projected.y / Projected.w / 2.0 + 0.5));
	[branch] if (Projected.z <= 0 || saturate(Coord.x) != Coord.x || saturate(Coord.y) != Coord.y)
		return float4(0, 0, 0, 0);

	float4 Color = float4(Materials[ob_Mid].Diffuse, 1.0);
	[branch] if (ob_Diffuse > 0)
		Color *= GetDiffuse(Coord * ob_TexCoord.xy);

	return Color;
};