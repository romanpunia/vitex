#include "vertex-in.hlsl"
#include "vertex-out.hlsl"
#include "deferred-out.hlsl"
#include "render-buffer.hlsl"

Texture2D Input1 : register(t1);
Texture2D Input2 : register(t2);
Texture2D Input3 : register(t3);
SamplerState State : register(s0);

VertexOut VS(VertexIn Input)
{
	VertexOut Output = (VertexOut)0;
	Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);
	Output.Normal = normalize(mul(Input.Normal, (float3x3)World));
	Output.TexCoord = Input.TexCoord * TexCoord;

	return Output;
}

DeferredOutput PS(VertexOut Input)
{
	float3 Diffuse = Diffusion;
	[branch] if (SurfaceDiffuse > 0)
		Diffuse *= Input1.Sample(State, Input.TexCoord).xyz;

	float2 Surface = Input3.SampleLevel(State, Input.TexCoord, 0).xy;

	DeferredOutput Output = (DeferredOutput)0;
	Output.Output1 = float4(Diffuse, Surface.x);
	Output.Output2 = float4(Input.Normal, Material);
	Output.Output3 = float2(Input.RawPosition.z / Input.RawPosition.w, Surface.y);

	return Output;
};