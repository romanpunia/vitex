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

    float Falloff = GetFalloff(Frag.Position);
    [branch] if (Falloff <= 0.0)
        return float4(0, 0, 0, 0);

    Material Mat = GetMaterial(Frag.Material);
    float R = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float4 Radiance = GetRadiance(Frag.Position, Frag.Normal, M);
    float4 Reflectance = GetReflectance(Frag.Position, Frag.Normal, M, R);
    float4 Result = VxIntensity * Radiance + Reflectance;

    return Result * Falloff;
};