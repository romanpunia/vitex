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
	float3 Shadow;
	float Iterations;
};

Texture2D Probe1 : register(t4);

float Pow4(float Value)
{
	return Value * Value * Value * Value;
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
	float3 A = 1.0f - length(D) / Range;
	[branch] if (Diffuse > 0)
		A *= Probe1.SampleLevel(State, P, 0).xyz;

	float3 V = normalize(ViewPosition.xyz - F.Position);
	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float3 E = CookTorrance(V, normalize(D), F.Normal, M, R, D) * A;

	return float4(Lighting * E, length(A) / 3.0f);
};