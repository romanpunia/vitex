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
	float Padding1;
	float3 Lighting;
	float Padding2;
};

float Pow4(float Value)
{
	return Value * Value * Value * Value;
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
	float3 D = Position;
	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float3 V = normalize(ViewPosition.xyz - F.Position);

	return float4(Lighting * CookTorrance(V, D, F.Normal, M, R, D), 1.0f);
};