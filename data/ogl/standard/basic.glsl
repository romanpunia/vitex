#include "renderer/vertex/shape"
#include "renderer/buffer/object"
#include "workflow/primitive"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, WorldViewProjection);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float4 Color = GetDiffuseLevel(V.TexCoord.xy, 0);
    return float4(Color.xyz * Diffuse, Color.w);
};