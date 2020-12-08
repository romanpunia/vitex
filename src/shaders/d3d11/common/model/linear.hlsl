#include "sdk/layouts/vertex"
#include "sdk/channels/gflux"
#include "sdk/buffers/object"

VOutputLinear VS(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(V.Position, WorldViewProjection);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}