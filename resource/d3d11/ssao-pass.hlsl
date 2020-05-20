#pragma warning(disable: 4000)
#include "shape-vertex-in.h"
#include "shape-vertex-out.hlsl"
#include "geometry.hlsl"
#include "view-buffer.hlsl"
#include "load-position.hlsl"
#include "load-texcoord.hlsl"

cbuffer RenderConstant : register(b3)
{
	float Scale;
	float Intensity;
	float Bias;
	float Radius;
	float Step;
	float Offset;
	float Distance;
	float Fading;
	float Power;
	float Samples;
	float SampleCount;
	float Padding;
}

float2 MakeRay(float2 TexCoord)
{
	float RayX = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f))) * 43758.5453f)) * 2.0f - 1.0f;
	float RayY = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f)) * 2.0f - 1.0f;
	return float2(RayX, RayY);
}

float Occlude(float2 TexCoord, float3 Position, float3 Normal)
{
	float3 Direction = LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection) - Position;
	float Length = length(Direction) * Scale;

	return max(0.0, dot(normalize(Direction), Normal) * Power - Bias) * Intensity / (1.0f + Length * Length);
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
	float2 TexCoord = LoadTexCoord(Input.TexCoord);
	float4 Normal = Input2.SampleLevel(State, TexCoord, 0);
	float3 Position = LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection);
	float Fade = saturate(pow(abs(distance(ViewPosition.xyz, Position) / Distance), Fading));
	float Factor = 0.0f;

	Material S = Materials[Normal.w];
	float AmbientRadius = Radius + S.Radius;

	[loop] for (int x = -Samples; x < Samples; x++)
	{
		[loop] for (int y = -Samples; y < Samples; y++)
		{
			float2 Ray = MakeRay(TexCoord) * Step - float2(y, x) * Offset;
			float2 Direction = reflect(Ray, float2(x, y)) * AmbientRadius;
			Factor += S.Occlusion * (Occlude(TexCoord + Direction, Position, Normal.xyz) +
				Occlude(TexCoord - Direction, Position, Normal.xyz));
		}
	}

	return float4(Input1.Sample(State, TexCoord).xyz * (1.0f - Factor * Fade / SampleCount), 1);
};