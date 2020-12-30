#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/lighting"
#include "sdk/core/material"
#include "sdk/core/position"

cbuffer RenderConstant : register(b3)
{
	matrix LightWorldViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float Distance;
    float Umbra;
	float Softness;
	float Bias;
	float Iterations;
};

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(V.Position, LightWorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

	Material Mat = GetMaterial(Frag.Material);
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 E = GetSurface(Frag, Mat);
    float3 K = Position - Frag.Position;
    float3 D = normalize(ViewPosition - Frag.Position);
    float3 L = normalize(K);
    float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
    float3 S = GetSubsurface(Frag.Normal, D, L, Mat.Scatter) * E;
    float A = GetRangeAttenuation(K, Range);

	return float4(Lighting * (R + S) * A, A);
};