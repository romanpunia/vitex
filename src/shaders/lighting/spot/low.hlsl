#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/lighting"
#include "std/core/material"
#include "std/core/position"
#include "lighting/spot/common/buffer"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), LightWorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

    float3 K = Position - Frag.Position;
    float3 L = normalize(K);
    float A = GetConeAttenuation(K, L, Attenuation.x, Attenuation.y, Range, Direction, Cutoff);
    [branch] if (A <= 0.0)
        return float4(0, 0, 0, 0);

	Material Mat = GetMaterial(Frag.Material);
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 E = GetSurface(Frag, Mat);
    float3 D = normalize(ViewPosition - Frag.Position);
    float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
    float3 S = GetSubsurface(Frag.Normal, D, L, Mat.Scatter) * E;

	return float4(Lighting * (R + S) * A, A);
};