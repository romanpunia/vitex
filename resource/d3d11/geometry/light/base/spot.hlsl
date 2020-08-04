#include "renderer/vertex/shape"
#include "workflow/pass"
#include "standard/cook-torrance"

cbuffer RenderConstant : register(b3)
{
	matrix OwnWorldViewProjection;
	matrix OwnViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float Diffuse;
	float3 Shadow;
	float Iterations;
};

Texture2D ProjectMap : register(t5);

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, OwnWorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

	float4 L = mul(float4(Frag.Position, 1), OwnViewProjection);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	[branch] if (L.z <= 0 || saturate(T.x) != T.x || saturate(T.y) != T.y)
		return float4(0, 0, 0, 0);

	Material Mat = GetMaterial(Frag.Material);
	float3 D = Position - Frag.Position;
	float3 A = 1.0 - length(D) / Range;
	[branch] if (Diffuse > 0)
		A *= GetSampleLevel(ProjectMap, T, 0).xyz;

	float3 P = normalize(ViewPosition - Frag.Position);
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float3 E = GetLight(P, normalize(D), Frag.Normal, M, R) * A;

	return float4(Frag.Diffuse * Lighting * E, length(A) / 3.0);
};