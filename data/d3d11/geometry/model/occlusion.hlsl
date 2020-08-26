#include "renderer/vertex"
#include "renderer/buffer/object"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, WorldViewProjection);

	return Result;
}