#include "renderer/vertex/gui"
#include "renderer/buffer/gui"
#include "workflow/primitive"

VertexResult VS(VertexBase V)
{
	VertexResult Result;
	Result.Position = mul(float4(V.Position.xy, 0.0, 1.0), WorldViewProjection);
	Result.Color = V.Color;
	Result.TexCoord = V.TexCoord;

	return Result;
};

float4 PS(VertexResult V) : SV_Target
{
	return V.Color * GetDiffuse(V.TexCoord);
};