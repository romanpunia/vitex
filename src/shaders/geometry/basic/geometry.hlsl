#include "std/layouts/shape"
#include "std/channels/immediate"
#include "std/buffers/object"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), WorldViewProjection);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
    float4 Color = GetDiffuse(V.TexCoord.xy);
    return float4(Color.xyz * Diffuse, Color.w);
};