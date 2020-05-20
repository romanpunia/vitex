sampler State;
Texture2D Diffuse;

cbuffer VertexBuffer : register(b0)
{
	float4x4 WorldViewProjection;
};

struct VertexIn
{
	float2 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

struct VertexOut
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

VertexOut VS(VertexIn Input)
{
	VertexOut Output;
	Output.Position = mul(WorldViewProjection, float4(Input.Position.xy, 0.f, 1.f));
	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;
	return Output;
};

float4 PS(VertexOut Input) : SV_Target
{
	return Input.Color * Diffuse.Sample(State, Input.TexCoord);
};