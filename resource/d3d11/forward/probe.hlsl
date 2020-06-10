#include "renderer/vertex/shape"
#include "geometry/sv"

cbuffer ProbeLight : register(b3)
{
	matrix OwnWorldViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float Mipping;
	float3 Scale;
	float Parallax;
};

TextureCube EnvironmentMap : register(t4);

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

	D = -normalize(reflect(normalize(ViewPosition.xyz - Frag.Position), -Frag.Normal));
	[branch] if (Parallax > 0)
	{
		float3 Max = Position + Scale;
		float3 Min = Position - Scale;
		float3 Plane = ((D > 0.0 ? Max : Min) - Frag.Position) / D;

		D = Frag.Position + D * min(min(Plane.x, Plane.y), Plane.z) - Position;
	}

	return float4(Lighting * GetSample3Level(EnvironmentMap, D, Mipping * R).xyz * M * A * (1 - R), A);
};