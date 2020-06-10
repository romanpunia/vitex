#include "renderer/vertex/shape"
#include "geometry/sv"
#include "standard/cook-torrance"

cbuffer PointLight : register(b3)
{
	matrix OwnWorldViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float Distance;
	float Softness;
	float Recount;
	float Bias;
	float Iterations;
};

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
	Material Mat = GetMaterial(Frag.Material);
	float3 D = Position - Frag.Position;
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float A = 1.0f - length(D) / Range;
	float3 P = normalize(ViewPosition.xyz - Frag.Position);

	return float4(Lighting * GetLight(P, normalize(D), Frag.Normal, M, R) * A, A);
};