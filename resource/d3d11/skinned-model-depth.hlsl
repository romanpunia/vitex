#include "influence-vertex-in.hlsl"
#include "influence-vertex-out.hlsl"
#include "animation-buffer.hlsl"
#include "render-buffer.hlsl"

Texture2D Input1 : register(t1);
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
	}
	else
		Output.Position = Output.RawPosition = mul(Input.Position, WorldViewProjection);

	return Output;
}

float4 PS(InfluenceVertexOut Input) : SV_TARGET0
{
	float Alpha = (1.0f - Diffusion.y);
	[branch] if (SurfaceDiffuse > 0)
		Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;

	[branch] if (Alpha < Diffusion.x)
		discard;

	return float4(Input.RawPosition.z / Input.RawPosition.w, Alpha, 1.0f, 1.0f);
};