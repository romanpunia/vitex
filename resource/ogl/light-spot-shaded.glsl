#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "view-buffer.hlsl"
#include "geometry.hlsl"
#include "load-geometry.hlsl"
#include "cook-torrance.hlsl"

cbuffer SpotLight : register(b3)
{
	matrix OwnWorldViewProjection;
	matrix OwnViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float Diffuse;
	float Softness;
	float Recount;
	float Bias;
	float Iterations;
};

Texture2D Probe1 : register(t4);
Texture2D Probe2 : register(t5);

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
			float2 Probe = Probe2.SampleLevel(State, D + float2(x, y) / Softness, 0).xy;
			C += step(L, Probe.x); B += Probe.y;
		}
	}

	C /= Recount; B /= Recount;
}

ShapeVertexOut VS(ShapeVertexIn Input)
{
	ShapeVertexOut Output = (ShapeVertexOut)0;
	Output.Position = mul(Input.Position, OwnWorldViewProjection);
	Output.TexCoord = Output.Position;
	return Output;
}

float4 PS(ShapeVertexOut Input) : SV_TARGET0
{
	Geometry F = LoadGeometrySV(InvViewProjection, Input.TexCoord);
	float4 L = mul(float4(F.Position, 1), OwnViewProjection);
	float2 P = float2(L.x / L.w / 2.0f + 0.5f, 1 - (L.y / L.w / 2.0f + 0.5f));
	[branch] if (L.z <= 0 || saturate(P.x) != P.x || saturate(P.y) != P.y)
		return float4(0, 0, 0, 0);

	Material S = Materials[F.Material];
	float3 D = Position - F.Position;
	float3 A = 1.0 - length(D) / Range;
	[branch] if (Diffuse > 0)
		A *= Probe1.SampleLevel(State, P, 0).xyz;

	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float3 V = normalize(ViewPosition.xyz - F.Position);
	float3 E = CookTorrance(V, normalize(D), F.Normal, M, R, D) * A;
	float I = L.z / L.w - Bias, C = 0.0, B = 0.0;

	[branch] if (Softness <= 0.0)
	{
		float2 Probe = Probe2.SampleLevel(State, P, 0).xy;
		C = step(I, Probe.x); B = Probe.y;
	}
	else
		SampleProbe(P, I, C, B);

	E *= C + B * (1.0 - C);
	return float4(Lighting * E, length(A) / 3.0f);
};