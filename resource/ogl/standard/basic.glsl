#include "renderer/vertex/shape"
#include "renderer/buffer/object"
#include "geometry/minimal"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, WorldViewProjection);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float4 Color = GetDiffuse(V.TexCoord.xy);
    return float4(Color.xyz * Diffuse, Color.w);
};