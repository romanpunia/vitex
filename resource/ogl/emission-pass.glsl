#pragma warning(disable: 4000)
#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "geometry.hlsl"
#include "view-buffer.hlsl"
#include "load-geometry.hlsl"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
	float Intensity;
	float Threshold;
	float2 Scaling;
	float Samples;
	float SampleCount;
}

ShapeVertexOut VS(ShapeVertexIn Input)
{
	ShapeVertexOut Output = (ShapeVertexOut)0;
	Output.Position = Input.Position;
	Output.TexCoord.xy = Input.TexCoord;
	return Output;
}

float4 PS(ShapeVertexOut Input) : SV_TARGET0
{
	Geometry F; Material S; float3 B = 0.0;
	[loop] for (int x = -Samples; x < Samples; x++)
	{
		[loop] for (int y = -Samples; y < Samples; y++)
		{
			float2 Offset = float2(x, y) / (Texel * Scaling);
			F = LoadGeometry(InvViewProjection, Input.TexCoord.xy + Offset);
			S = Materials[F.Material];

			B += saturate(F.Diffuse + S.Emission * S.Intensity - Threshold) * Intensity;
		}
	}

	return float4(F.Diffuse + B / SampleCount, 1.0f);
};