#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/lighting"
#include "sdk/core/material"
#include "sdk/core/position"

cbuffer RenderConstant : register(b3)
{
	matrix LightWorldViewProjection;
	matrix LightViewProjection;
    float3 Direction;
    float Cutoff;
	float3 Position;
	float Range;
	float3 Lighting;
	float Softness;
	float Bias;
	float Iterations;
	float Umbra;
    float Padding;
};

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), LightWorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

    float3 K = Position - Frag.Position;
    float3 L = normalize(K);
    float A = GetConeAttenuation(K, L, Range, Direction, Cutoff);
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