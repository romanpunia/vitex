#include "renderer/element"
#include "renderer/vertex/element"
#include "renderer/buffer/object"
#include "renderer/input/element"
#include "workflow/rasterizer"

VertexResult90 Make(in VertexResult90 V, in float2 Offset, in float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, World);
	V.TexCoord = TexCoord2;
	return V;
}

[maxvertexcount(4)]
void GS(point VertexResult90 V[1], inout TriangleStream<VertexResult90> Stream)
{
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VertexResult90 VS(VertexBase V)
{
	VertexResult90 Result = (VertexResult90)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), WorldViewProjection);
	Result.UV = mul(Result.Position, World);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Scale = Elements[V.Position].Scale;
	Result.Alpha = Elements[V.Position].Color.w;

	return Result;
}

float4 PS(VertexResult90 V) : SV_TARGET0
{
    Material Mat = GetMaterial(MaterialId);
	float Alpha = V.Alpha * (1.0 - Mat.Limpidity);

	[branch] if (HasDiffuse > 0)
		Alpha *= GetDiffuse(V.TexCoord).w;

	return float4(V.UV.z / V.UV.w, 1.0 - Alpha, 1.0, 1.0);
};