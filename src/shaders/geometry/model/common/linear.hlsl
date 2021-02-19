#include "std/layouts/vertex"
#include "std/channels/depth"
#include "std/buffers/object"

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), WorldViewProjection);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}