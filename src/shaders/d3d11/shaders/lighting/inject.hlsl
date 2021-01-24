#include "sdk/layouts/shape"
#include "sdk/channels/raytracer"
#include "sdk/core/position"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(1, 0, 0, 1.0);

	return float4(Frag.Normal, 1.0);
};