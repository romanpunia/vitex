#include "sdk/layouts/shape"
#include "sdk/channels/raytracer"
#include "sdk/core/raytracing"
#include "sdk/core/position"
#include "sdk/core/lighting"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment2D(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

    Material Mat = GetMaterial(Frag.Material);
    float R = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
	float A = GetAperture(R);
    float3 E = normalize(Frag.Position - ViewPosition);
	float3 D = reflect(E, Frag.Normal);
    float4 Result = Conetrace(Frag.Normal, Frag.Position, D, A);

	return float4(GetSpecularBRDF(Frag.Normal, -E, D, Result.xyz, M, R), Result.w);
};