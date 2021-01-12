#include "sdk/layouts/vertex"
#include "sdk/buffers/object"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), WorldViewProjection);

	return Result;
}