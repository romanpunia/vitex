#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "view-buffer.hlsl"
#include "geometry.hlsl"
#include "load-geometry.hlsl"

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

TextureCube Probe1 : register(t4);

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
	Material S = Materials[F.Material];
	float3 D = Position - F.Position;
	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float A = 1.0f - length(D) / Range;

	D = -normalize(reflect(normalize(ViewPosition.xyz - F.Position), -F.Normal));
	[branch] if (Parallax > 0)
	{
		float3 Max = Position + Scale;
		float3 Min = Position - Scale;
		float3 Plane = ((D > 0.0 ? Max : Min) - F.Position) / D;

		D = F.Position + D * min(min(Plane.x, Plane.y), Plane.z) - Position;
	}

	return float4(Lighting * Probe1.SampleLevel(State, D, Mipping * R).xyz * M * A * S.Reflectance, A);
};