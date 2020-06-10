#include "renderer/vertex/gui"
#include "renderer/buffer/gui"
#include "geometry/minimal"

VertexResult VS(VertexBase V)
{
	VertexResult Result;
	Result.Position = mul(float4(V.Position.xy, 0.0f, 1.0f), WorldViewProjection);
	Result.Color = V.Color;
	Result.TexCoord = V.TexCoord;

	return Result;
};

float4 PS(VertexResult V) : SV_Target
{
	return V.Color * GetDiffuse(V.TexCoord);
};