#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "geometry.hlsl"
#include "view-buffer.hlsl"
#include "load-geometry.hlsl"
#include "load-position.hlsl"
#include "load-texcoord.hlsl"
#include "cook-torrance.hlsl"

cbuffer RenderConstant : register(b3)
{
	matrix ViewProjection;
	float IterationCount;
	float RayCorrection;
	float RayLength;
	float MipLevels;
}

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
	float4 Offset = float4(0, 0, 0, 0);
	float3 Eye = normalize(F.Position - ViewPosition.xyz);
	float3 Ray = reflect(Eye, F.Normal), Sample;
	float3 Angle = pow(abs(1 - dot(-Eye, F.Normal)), 2);
	float3 M = S.Metallic + F.Surface.x * S.Micrometal;
	float R = Pow4(S.Roughness + F.Surface.y * S.Microrough);
	float Depth = RayLength;

	[branch] if (S.Reflection <= 0)
		return float4(F.Diffuse, 1.0f);

	[loop] for (int i = 0; i < IterationCount; i++)
	{
		Offset = mul(float4(F.Position + Ray * Depth, 1.0f), ViewProjection);
		Offset.xy = float2(0.5f, 0.5f) + float2(0.5f, -0.5f) * Offset.xy / Offset.w;

		Depth = Input3.SampleLevel(State, Offset.xy, 0).r;
		[branch] if (Depth < RayCorrection)
			return float4(F.Diffuse, 1.0f);

		Sample = LoadPosition(F.TexCoord, Input3.SampleLevel(State, Offset.xy, 0).r, InvViewProjection);
		Depth = length(F.Position - Sample);
	}

	Ray = Input1.SampleLevel(State, Offset.xy, MipLevels * R).xyz;
	return float4(F.Diffuse + Ray * Angle * M * S.Reflection, 1.0f);
};