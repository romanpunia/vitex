#include "standard/space-sv"
#include "standard/cook-torrance"
#pragma warning(disable: 3595)

cbuffer LineLight : register(b3)
{
	matrix OwnViewProjection;
	float3 Position;
	float ShadowDistance;
	float3 Lighting;
	float ShadowLength;
	float Softness;
	float Recount;
	float Bias;
	float Iterations;
};

Texture2D ShadowMap : register(t4);

void SampleShadow(in float2 D, in float L, out float C, out float B)
{
	[loop] for (int x = -Iterations; x < Iterations; x++)
	{
		[loop] for (int y = -Iterations; y < Iterations; y++)
		{
			float2 Shadow = GetSampleLevel(ShadowMap, D + float2(x, y) / Softness, 0).xy;
			C += step(L, Shadow.x); B += Shadow.y;
		}
	}

	C /= Recount; B /= Recount;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
	Material Mat = GetMaterial(Frag.Material);
	float4 L = mul(float4(Frag.Position, 1), OwnViewProjection);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	float3 D = Position;
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float3 P = normalize(ViewPosition - Frag.Position);
	float3 E = GetLight(P, D, Frag.Normal, M, R);

	[branch] if (saturate(T.x) == T.x && saturate(T.y) == T.y)
	{
		float G = length(float2(ViewPosition.x - P.x, ViewPosition.z - P.z));
		float I = L.z / L.w - Bias, C = 0.0, B = 0.0;

		[branch] if (Softness <= 0.0)
		{
			float2 Shadow = GetSampleLevel(ShadowMap, T, 0).xy;
			C = step(I, Shadow.x); B = Shadow.y;
		}
		else
			SampleShadow(T, I, C, B);

		C = saturate(C + saturate(pow(abs(G / ShadowDistance), ShadowLength)));
		E *= C + B * (1.0 - C);
	}

	return float4(Lighting * E, 1.0);
};