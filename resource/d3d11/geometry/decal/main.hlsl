#include "renderer/vertex"
#include "renderer/buffer/object"
#include "workflow/geometry"

cbuffer RenderConstant : register(b3)
{
	matrix OwnViewProjection;
};

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = Result.UV = mul(V.Position, WorldViewProjection);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.Tangent = normalize(mul(V.Tangent, (float3x3)World));
	Result.Bitangent = normalize(mul(V.Bitangent, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}

GDepthless PS(VertexResult V)
{
	float4 L = mul(float4(V.Position.xyz, 1), OwnViewProjection);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	[branch] if (L.z <= 0 || saturate(T.x) != T.x || saturate(T.y) != T.y)
        discard;

	float4 Color = float4(Diffuse, 1.0);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(T);

	float3 Normal = V.Normal;
	[branch] if (HasNormal > 0)
        Normal = GetNormal(T, V.Normal, V.Tangent, V.Bitangent);

    return ComposeDepthless(T, Color, Normal, MaterialId);
};