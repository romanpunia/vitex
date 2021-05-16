#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/sampler"

Texture2D Image : register(t5);

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{    
    float3 A = GetDiffuse(V.TexCoord.xy, 0).xyz;
    float3 B = GetSampleLevel(Image, V.TexCoord.xy, 0).xyz;
    return float4(A * B, 1.0);
};