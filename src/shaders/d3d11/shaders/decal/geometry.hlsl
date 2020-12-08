#include "sdk/layouts/shape"
#include "sdk/channels/geometry"
#include "sdk/buffers/object"
#include "sdk/buffers/viewer"
#include "sdk/core/sampler"

cbuffer RenderConstant : register(b3)
{
	matrix DecalViewProjection;
};

Texture2D LChannel2 : register(t8);

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(V.Position, WorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    float2 TexCoord = float2(0.5 + 0.5 * V.TexCoord.x / V.TexCoord.w, 0.5 - 0.5 * V.TexCoord.y / V.TexCoord.w);
    float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, GetSampleLevel(LChannel2, TexCoord, 0).x, 1.0), InvViewProjection);
	float4 Projected = mul(Position / Position.w, DecalViewProjection);

	TexCoord = float2(Projected.x / Projected.w / 2.0 + 0.5, 1 - (Projected.y / Projected.w / 2.0 + 0.5));
	[branch] if (Projected.z <= 0 || saturate(TexCoord.x) != TexCoord.x || saturate(TexCoord.y) != TexCoord.y)
        return float4(0, 0, 0, 0);

	float4 Color = float4(Diffuse, 1.0);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(TexCoord);

    return Color;
};