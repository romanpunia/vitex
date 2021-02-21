#include "std/layouts/vertex"
#include "std/channels/depth"
#include "std/buffers/object"

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), ob_WorldViewProj);
	Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	return Result;
}