#include "renderer/vertex/shape"
#include "renderer/buffer/object"
#include "workflow/geometry"

cbuffer RenderConstant : register(b3)
{
	matrix DecalViewProjection;
};

Texture2D LChannel2 : register(t8);

float3 GetWorldPosition(in float4 UV)
{
    float2 TexCoord = float2(0.5f + 0.5f * UV.x / UV.w, 0.5f - 0.5f * UV.y / UV.w);
    float Depth = GetSampleLevel(LChannel2, TexCoord, 0).x;
    float4 Position = float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, Depth, 1.0);
    Position = mul(Position, InvViewProjection);

    return Position.xyz / Position.w;
}

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, WorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	float4 L = mul(float4(GetWorldPosition(V.TexCoord), 1), DecalViewProjection);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	[branch] if (L.z <= 0 || saturate(T.x) != T.x || saturate(T.y) != T.y)
        discard;

	float4 Color = float4(Diffuse, 1.0);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(T);

    return Color;
};