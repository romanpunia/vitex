#include "standard/space-sv"
#include "standard/cook-torrance"
#include "standard/hemi-ambient"

cbuffer LineLight : register(b3)
{
	matrix OwnViewProjection;
	float3 Position;
	float Padding1;
	float3 Lighting;
	float Padding2;
};

float4 PS(VertexResult V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
	Material Mat = GetMaterial(Frag.Material);

	float3 D = Position;
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float3 P = normalize(ViewPosition - Frag.Position);
    
	return float4(Lighting * GetLight(P, D, Frag.Normal, M, R), 1.0);
};