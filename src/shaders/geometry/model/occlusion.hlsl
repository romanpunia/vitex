#include "std/layouts/vertex"
#include "std/buffers/object"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), WorldViewProjection);

	return Result;
}