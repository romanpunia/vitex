#include "renderer/vertex/shape"
#include "workflow/effects"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = V.Position;
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}