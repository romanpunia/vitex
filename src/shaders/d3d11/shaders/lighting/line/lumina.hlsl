#include "sdk/layouts/shape"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = V.Position;
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	return float4(1.0, 1.0, 1.0, 1.0);
};