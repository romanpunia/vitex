#include "influence-vertex-in.hlsl"
#include "influence-vertex-out.hlsl"
#include "deferred-out.hlsl"
#include "animation-buffer.hlsl"
#include "render-buffer.hlsl"

Texture2D Input1 : register(t1);
Texture2D Input2 : register(t2);
Texture2D Input3 : register(t3);
SamplerState State : register(s0);

InfluenceVertexOut VS(InfluenceVertexIn Input)
{
	InfluenceVertexOut Output = (InfluenceVertexOut)0;
	Output.TexCoord = Input.TexCoord * TexCoord;

	[branch] if (Base.x > 0)
	{
		matrix Offset =
			mul(Transformation[(int)Input.Index.x], Input.Bias.x) +
			mul(Transformation[(int)Input.Index.y], Input.Bias.y) +
			mul(Transformation[(int)Input.Index.z], Input.Bias.z) +
			mul(Transformation[(int)Input.Index.w], Input.Bias.w);

		Output.Position = Output.RawPosition = mul(mul(Input.Position, Offset), WorldViewProjection);
		Output.Normal = normalize(mul(mul(float4(Input.Normal, 0), Offset).xyz, (float3x3)World));
		Output.Tangent = normalize(mul(mul(float4(Input.Tangent, 0), Offset).xyz, (float3x3)World));
		Output.Bitangent = normalize(mul(mul(float4(Input.Bitangent, 0), Offset).xyz, (float3x3)World));
	}
	else
	{
		Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);
		Output.Normal = normalize(mul(Input.Normal, (float3x3)World));
		Output.Tangent = normalize(mul(Input.Tangent, (float3x3)World));
		Output.Bitangent = normalize(mul(Input.Bitangent, (float3x3)World));
	}

	return Output;
}

DeferredOutput PS(InfluenceVertexOut Input)
{
	float3 Diffuse = Diffusion;
	[branch] if (SurfaceDiffuse > 0)
		Diffuse *= Input1.Sample(State, Input.TexCoord).xyz;

	float3 Normal = Input.Normal;
	[branch] if (SurfaceNormal > 0)
	{
		Normal = Input2.Sample(State, Input.TexCoord).xyz * 2.0f - 1.0f;
		Normal = normalize(Normal.x * Input.Tangent + Normal.y * Input.Bitangent + Normal.z * Input.Normal);
	}

	float2 Surface = Input3.SampleLevel(State, Input.TexCoord, 0).xy;

	DeferredOutput Output = (DeferredOutput)0;
	Output.Output1 = float4(Diffuse, Surface.x);
	Output.Output2 = float4(Normal, Material);
	Output.Output3 = float2(Input.RawPosition.z / Input.RawPosition.w, Surface.y);

	return Output;
};