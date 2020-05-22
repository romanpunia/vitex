#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "view-buffer.hlsl"
#include "geometry.hlsl"
#include "load-geometry.hlsl"
#include "cook-torrance.hlsl"

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

TextureCube Probe1 : register(t4);

float Pow4(float Value)
{
	return Value * Value * Value * Value;
}

void SampleProbe(float3 D, float L, out float C, out float B)
{
	for (int x = -Iterations; x < Iterations; x++)
	{
		for (int y = -Iterations; y < Iterations; y++)
		{
			for (int z = -Iterations; z < Iterations; z++)
			{
				float2 Probe = Probe1.SampleLevel(State, D + float3(x, y, z) / Softness, 0).xy;
				C += step(L, Probe.x); B += Probe.y;
			}
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
	Material S = Materials[F.Material];
	float3 D = Position - F.Position;
	float3 K = normalize(D);
	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float L = length(D);
	float A = 1.0f - L / Range;
	float3 V = normalize(ViewPosition.xyz - F.Position);
	float I = L / Distance - Bias, C = 0.0, B = 0.0;

	float3 E = CookTorrance(V, K, F.Normal, M, R, D) * A;
	[branch] if (Softness <= 0.0)
	{
		float2 Probe = Probe1.SampleLevel(State, -K, 0).xy;
		C = step(I, Probe.x); B = Probe.y;
	}
	else
		SampleProbe(-K, I, C, B);

	E *= C + B * (1.0 - C);
	return float4(Lighting * E, A);
};