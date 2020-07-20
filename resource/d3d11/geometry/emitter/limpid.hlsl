#include "renderer/element"
#include "renderer/vertex/element"
#include "renderer/buffer/object"
#include "renderer/input/element"
#include "workflow/geometry"

VertexResult Make(in VertexResult V, in float2 Offset, in float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, World);
	V.TexCoord = TexCoord2;
	return V;
}

[maxvertexcount(4)]
void GS(point VertexResult V[1], inout TriangleStream<VertexResult> Stream)
{
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), WorldViewProjection);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Color = Elements[V.Position].Color;
	Result.Scale = Elements[V.Position].Scale;
    
	return Result;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float4 Color = float4(V.Color.xyz * Diffuse, V.Color.w);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	return Color;
};