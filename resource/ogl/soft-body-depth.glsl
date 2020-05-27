#include "vertex-in.glsl"
#include "render-buffer.glsl"

Texture2D Input1 : register(t1);
SamplerState State : register(s0);

struct VertexOut
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 RawPosition : TEXCOORD1;
};

VertexOut VS(VertexIn Input)
{
	VertexOut Output = (VertexOut)0;
	Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);
	Output.TexCoord = Input.TexCoord * TexCoord;
	return Output;
}

float4 PS(VertexOut Input) : SV_TARGET0
{
	float Alpha = (1.0f - Diffusion.y);
	[branch] if (SurfaceDiffuse > 0)
		Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;

	[branch] if (Alpha < Diffusion.x)
		discard;

	return float4(Input.RawPosition.z / Input.RawPosition.w, Alpha, 1.0f, 1.0f);
};