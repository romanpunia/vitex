#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/lighting"
#include "sdk/core/raymarching"
#include "sdk/core/material"
#include "sdk/core/position"

cbuffer RenderConstant : register(b3)
{
	float Samples;
	float MipLevels;
	float Intensity;
	float Padding;
}

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
        return float4(0.0, 0.0, 0.0, 1.0);

	float3 E = normalize(Frag.Position - ViewPosition);
	float3 D = reflect(E, Frag.Normal);
    float3 HitCoord;

    [branch] if (!RayMarch(Frag.Position, D, Samples, HitCoord))
        return float4(0.0, 0.0, 0.0, 1.0);

	Material Mat = GetMaterial(Frag.Material);
    float T = GetRoughnessMip(Frag, Mat, 1.0);
    float R = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 L = GetDiffuse(HitCoord.xy, T * MipLevels).xyz * Intensity;
    float3 C = GetSpecularBRDF(Frag.Normal, -E, normalize(D), L, M, R);
    float P = length(GetPosition(HitCoord.xy, HitCoord.z) - Frag.Position) * T;
    float A = RayEdge(HitCoord.xy) / (1.0 + 6.0 * P * P);
    
	return float4(C * A, 1.0);
};