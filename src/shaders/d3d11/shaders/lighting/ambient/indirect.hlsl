#include "sdk/layouts/shape"
#include "sdk/channels/raytracer"
#include "sdk/core/raytracing"
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
        return float4(0, 0, 0, 0);
        
	float3 E = normalize(Frag.Position - ViewPosition);
	float3 D = reflect(E, Frag.Normal);
    float3 C = ConeTraceDiffuse(Frag.Position, D);

	return float4(Frag.Normal, 1.0);
};