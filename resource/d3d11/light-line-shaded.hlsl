#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "view-buffer.hlsl"
#include "geometry.hlsl"
#include "load-geometry.hlsl"
#include "cook-torrance.hlsl"

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

Texture2D Probe1 : register(t4);

float Pow4(float Value)
{
	return Value * Value * Value * Value;
}

void SampleProbe(float2 D, float L, out float C, out float B)
{
	for (int x = -Iterations; x < Iterations; x++)
	{
		for (int y = -Iterations; y < Iterations; y++)
		{
			float2 Probe = Probe1.SampleLevel(State, D + float2(x, y) / Softness, 0).xy;
			C += step(L, Probe.x); B += Probe.y;
		}
	}

	C /= Recount; B /= Recount;
}

ShapeVertexOut VS(ShapeVertexIn Input)
{
	ShapeVertexOut Output = (ShapeVertexOut)0;
	Output.Position = Input.Position;
	Output.TexCoord = Output.Position;
	return Output;
}

float4 PS(ShapeVertexOut Input) : SV_TARGET0
{
	Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);
	Material S = Materials[F.Material];
	float4 L = mul(float4(F.Position, 1), OwnViewProjection);
	float2 P = float2(L.x / L.w / 2.0f + 0.5f, 1 - (L.y / L.w / 2.0f + 0.5f));
	float3 D = Position;
	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float3 V = normalize(ViewPosition.xyz - F.Position);
	float3 E = CookTorrance(V, D, F.Normal, M, R, D);

	[branch] if (saturate(P.x) == P.x && saturate(P.y) == P.y)
	{
		float G = length(float2(ViewPosition.x - V.x, ViewPosition.z - V.z));
		float I = L.z / L.w - Bias, C = 0.0, B = 0.0;

		[branch] if (Softness <= 0.0)
		{
			float2 Probe = Probe1.SampleLevel(State, P, 0).xy;
			C = step(I, Probe.x); B = Probe.y;
		}
		else
			SampleProbe(P, I, C, B);

		C = saturate(C + saturate(pow(abs(G / ShadowDistance), ShadowLength)));
		E *= C + B * (1.0f - C);
	}

	return float4(Lighting * E, 1.0f);
};