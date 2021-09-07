#include "std/layouts/shape.hlsl"
#include "std/channels/immediate.hlsl"
#include "std/buffers/object.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), ob_WorldViewProj);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	float4 Color = GetDiffuse(V.TexCoord.xy);
	return float4(Color.xyz * ob_TexCoord.xyz, Color.w);
};