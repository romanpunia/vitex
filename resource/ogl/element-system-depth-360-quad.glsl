#include "instance-element.hlsl"
#include "render-buffer.hlsl"

Texture2D Input1 : register(t1);
SamplerState State : register(s0);
StructuredBuffer<InstanceElement> Elements : register(t2);

cbuffer ViewBuffer : register(b3)
{
	matrix SliceView[6];
	matrix Projection;
	float3 Position;
	float Distance;
};

struct VertexIn
{
	uint Position : SV_VERTEXID;
};

struct VertexCubeOut
{
	float4 Position : SV_POSITION;
	float4 RawPosition : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float Rotation : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Alpha : TEXCOORD4;
	uint RenderTarget : SV_RenderTargetArrayIndex;
};

struct VertexOut
{
	float4 Position : SV_POSITION;
	float4 RawPosition : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float Rotation : TEXCOORD2;
	float Scale : TEXCOORD3;
	float Alpha : TEXCOORD4;
};

VertexCubeOut TransformElement(VertexOut Input, float2 Offset, float2 TexCoord2, uint i)
{
	float Sin = sin(Input.Rotation), Cos = cos(Input.Rotation);
	Input.Position = mul(Input.Position, SliceView[i]);
	Input.Position += float4(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos, 0, 0);
	Input.Position = mul(Input.Position, Projection);

	VertexCubeOut Output = (VertexCubeOut)0;
	Output.Position = Input.Position;
	Output.RawPosition = Input.RawPosition;
	Output.Rotation = Input.Rotation;
	Output.Scale = Input.Scale;
	Output.Alpha = Input.Alpha;
	Output.RenderTarget = i;
	Output.TexCoord = TexCoord2;

	return Output;
}

[maxvertexcount(24)]
void GS(point VertexOut Input[1], inout TriangleStream<VertexCubeOut> Stream)
{
	for (uint i = 0; i < 6; i++)
	{
		Stream.Append(TransformElement(Input[0], float2(1, -1) * Input[0].Scale, float2(0, 0), i));
		Stream.Append(TransformElement(Input[0], float2(1, 1) * Input[0].Scale, float2(0, -1), i));
		Stream.Append(TransformElement(Input[0], float2(-1, -1) * Input[0].Scale, float2(1, 0), i));
		Stream.Append(TransformElement(Input[0], float2(-1, 1) * Input[0].Scale, float2(1, -1), i));
		Stream.RestartStrip();
	}
}

VertexOut VS(VertexIn Input)
{
	VertexOut Output = (VertexOut)0;
	Output.Position = Output.RawPosition = mul(float4(Elements[Input.Position].Position, 1), World);
	Output.Rotation = Elements[Input.Position].Rotation;
	Output.Scale = Elements[Input.Position].Scale;
	Output.Alpha = Elements[Input.Position].Color.w;
	return Output;
}

float4 PS(VertexOut Input) : SV_TARGET0
{
	float Alpha = (1.0f - Diffusion.y) * Input.Alpha;
	[branch] if (SurfaceDiffuse > 0)
		Alpha *= Input1.Sample(State, Input.TexCoord * TexCoord).w;

	[branch] if (Alpha < Diffusion.x)
		discard;

	return float4(length(Input.RawPosition.xyz - Position) / Distance, Alpha, 1.0f, 1.0f);
};