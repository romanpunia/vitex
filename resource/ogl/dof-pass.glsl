#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "geometry.hlsl"
#include "view-buffer.hlsl"
#include "load-geometry.hlsl"
#include "load-position.hlsl"
#include "load-texcoord.hlsl"

cbuffer RenderConstant : register(b3)
{
	matrix ViewProjection;
	float2 Texel;
	float Threshold;
	float Gain;
	float Fringe;
	float Bias;
	float Dither;
	float Samples;
	float Rings;
	float FarDistance;
	float FarRange;
	float NearDistance;
	float NearRange;
	float FocalDepth;
	float Intensity;
	float Circular;
}

float3 Aberate(float2 TexCoord, float Weight)
{
	float3 Coefficient = float3(0.299, 0.587, 0.114);
	float3 Aberation = float3(Input1.SampleLevel(State, TexCoord + float2(0.0, 1.0) * Texel * Fringe * Weight, 0).r,
	Input1.SampleLevel(State, TexCoord + float2(-0.866, -0.5) * Texel * Fringe * Weight, 0).g,
	Input1.SampleLevel(State, TexCoord + float2(0.866, -0.5) * Texel * Fringe * Weight, 0).b);
	float Luminance = dot(Aberation * Intensity, Coefficient);

	return Aberation + lerp(0, Aberation, max((Luminance - Threshold) * Gain, 0.0) * Weight);
}

float2 Random(float2 TexCoord)
{
	float NoiseX = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f))) * 43758.5453f)) * 2.0f - 1.0f;
	float NoiseY = saturate(frac(sin(dot(TexCoord, float2(12.9898f, 78.233f) * 2.0f)) * 43758.5453f)) * 2.0f - 1.0f;
	return float2(NoiseX, NoiseY);
}

float Weight(float2 TexCoord)
{
	float4 Position = float4(LoadPosition(TexCoord, Input3.SampleLevel(State, TexCoord, 0).r, InvViewProjection), 1);
	Position = mul(Position, ViewProjection);

	float Alpha = 1.0f - (Position.z - NearDistance) / NearRange;
	float Gamma = (Position.z - (FarDistance - FarRange)) / FarRange;
	return saturate(max(Alpha, Gamma)) * FocalDepth;
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
	float Amount = Weight(TexCoord);
	float3 Color = Input1.Sample(State, TexCoord).rgb;
	float2 Noise = Random(TexCoord) * Dither * Amount;
	float Width = Texel.x * Amount + Noise.x;
	float Height = Texel.y * Amount + Noise.y;
	float SampleAmount = 1.0;

	[loop] for (int i = 1; i <= Rings; i++)
	{
		float Count = i * Samples;
		[loop] for (int j = 0; j < Count; j++)
		{
			float Step = 6.28318530f / Count;
			float2 Fragment = TexCoord + float2(cos(j * Step) * i * Width, sin(j * Step) * i * Height);

			[branch] if (Weight(Fragment) >= Amount)
			{
				Color += Aberate(Fragment, Amount) * lerp(1.0f, (float)i / Rings, Bias) * Circular;
				SampleAmount += lerp(1.0f, (float)i / Rings, Bias);
			}
		}
	}

	Color /= SampleAmount;
	return float4(Color, 1);
};