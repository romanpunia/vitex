#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "view-buffer.hlsl"
#include "geometry.hlsl"
#include "hemi-ambient.hlsl"

Texture2D Input4 : register(t4);

cbuffer AmbientLight : register(b3)
{
	float3 HighEmission;
	float Padding1;
	float3 LowEmission;
	float Padding2;
};

ShapeVertexOut VS(ShapeVertexIn Input)
{
	ShapeVertexOut Output = (ShapeVertexOut)0;
	Output.Position = Input.Position;
	Output.TexCoord.xy = Input.TexCoord;
	return Output;
}

float4 PS(ShapeVertexOut Input) : SV_TARGET0
{
	float4 Diffusion = Input1.Sample(State, Input.TexCoord.xy);
	float4 Emission = Input4.Sample(State, Input.TexCoord.xy);
	float4 Normal = Input2.Sample(State, Input.TexCoord.xy);
	float3 Ambient = HemiAmbient(HighEmission, LowEmission, Normal.y);

	return float4(Diffusion.xyz * (Emission.xyz + Ambient) + Emission.xyz * Emission.a, 1.0f);
};