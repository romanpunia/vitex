#include "sdk/layouts/vertex"
#include "sdk/buffers/object"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(V.Position, WorldViewProjection);

	return Result;
}