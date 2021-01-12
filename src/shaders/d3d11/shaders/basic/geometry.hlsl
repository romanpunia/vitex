#include "sdk/layouts/shape"
#include "sdk/channels/immediate"
#include "sdk/buffers/object"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), WorldViewProjection);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    float4 Color = GetDiffuse(V.TexCoord.xy);
    return float4(Color.xyz * Diffuse, Color.w);
};