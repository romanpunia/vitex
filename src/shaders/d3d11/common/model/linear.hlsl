#include "sdk/layouts/vertex"
#include "sdk/channels/depth"
#include "sdk/buffers/object"

VOutputLinear VS(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), WorldViewProjection);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}