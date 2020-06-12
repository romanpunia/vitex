#include "renderer/vertex/shape"
#include "geometry/sv"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = V.Position;
	Result.TexCoord = Result.Position;

	return Result;
}