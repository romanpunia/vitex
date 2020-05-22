#include "instance-element.hlsl"
#include "render-buffer.hlsl"

Texture2D Input1 : register(t1);
SamplerState State : register(s0);
StructuredBuffer<InstanceElement> Elements : register(t2);

struct VertexIn
{
	uint Position : SV_VERTEXID;
};

struct VertexOut
{
	float4 Position : SV_POSITION;
	float4 Color : TEXCOORD0;
	float2 TexCoord : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Rotation : TEXCOORD4;
};

VertexOut TransformElement(VertexOut Input, float2 Offset, float2 TexCoord2)
{
	float Sin = sin(Input.Rotation), Cos = cos(Input.Rotation);
	Input.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	Input.Position = mul(Input.Position, World);
	Input.TexCoord = TexCoord2;
	return Input;
}

[maxvertexcount(4)]
void GS(point VertexOut Input[1], inout TriangleStream<VertexOut> Stream)
{
	Stream.Append(TransformElement(Input[0], float2(-1, -1) * Input[0].Scale, float2(0, 0)));
	Stream.Append(TransformElement(Input[0], float2(-1, 1) * Input[0].Scale, float2(0, -1)));
	Stream.Append(TransformElement(Input[0], float2(1, -1) * Input[0].Scale, float2(1, 0)));
	Stream.Append(TransformElement(Input[0], float2(1, 1) * Input[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VertexOut VS(VertexIn Input)
{
	VertexOut Output = (VertexOut)0;
	Output.Position = mul(float4(Elements[Input.Position].Position, 1), WorldViewProjection);
	Output.Rotation = Elements[Input.Position].Rotation;
	Output.Color = Elements[Input.Position].Color;
	Output.Scale = Elements[Input.Position].Scale;
	return Output;
}

float4 PS(VertexOut Input) : SV_TARGET0
{
	[branch] if (SurfaceDiffuse > 0)
		return float4(Input.Color.xyz * Diffusion, Input.Color.w) * Input1.Sample(State, Input.TexCoord * TexCoord);

	return float4(Input.Color.xyz * Diffusion, Input.Color.w);
};