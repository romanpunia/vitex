#include "renderer/element"
#include "renderer/vertex/element"
#include "renderer/buffer/object"
#include "renderer/input/element"
#include "workflow/geometry"

VertexResultOPQ Make(VertexResultOPQ V, in float2 Offset, in float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, World);
    V.UV = V.Position;
	V.TexCoord = TexCoord2;
	return V;
}

[maxvertexcount(4)]
void GS(point VertexResultOPQ V[1], inout TriangleStream<VertexResultOPQ> Stream)
{
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VertexResultOPQ VS(VertexBase V)
{
	VertexResultOPQ Result = (VertexResultOPQ)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), WorldViewProjection);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Color = Elements[V.Position].Color;
	Result.Scale = Elements[V.Position].Scale;
    Result.UV = Result.Position;
    Result.Normal = float3(0, 0, 1);
    Result.Tangent = float3(1, 0, 0);
    Result.Bitangent = float3(0, -1, 0);
    
	return Result;
}

GBuffer PS(VertexResultOPQ V)
{
	float4 Color = float4(Diffuse * V.Color.xyz, V.Color.w);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	float3 Normal = V.Normal;
	[branch] if (HasNormal > 0)
        Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);

    return Compose(V.TexCoord, Color, Normal, V.UV.z / V.UV.w, MaterialId);
};