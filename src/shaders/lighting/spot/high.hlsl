#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/lighting"
#include "std/core/material"
#include "std/core/random"
#include "std/core/position"
#include "lighting/spot/common/buffer"
#pragma warning(disable: 4000)

Texture2D ShadowMap : register(t5);
SamplerState ShadowSampler : register(s1);

float GetPenumbra(float2 D, float L)
{
    [branch] if (Umbra <= 0.0)
        return 0.0;
    
    float Length = 0.0, Count = 0.0;
    [unroll] for (float i = 0; i < 16; i++)
    {
        float S1 = ShadowMap.SampleLevel(ShadowSampler, D + FiboDisk[i] / Softness, 0).x;
        float S2 = step(S1, L);
        Length += S1 * S2;
        Count += S2;
    }

    [branch] if (Count < 2.0)
        return 1.0;
    
    Length /= Count;
    return saturate(Umbra * FarPlane * (L - Length) / Length);
}
float GetLightness(float2 D, float L)
{
    float Penumbra = GetPenumbra(D, L);
    [branch] if (Penumbra >= 1.0)
        return 1.0;

    float Result = 0.0;
	[loop] for (float j = 0; j < Iterations; j++)
    {
        float2 Offset = FiboDisk[j % 64] * (64.0 / max(64.0, j));
        Result += step(L, ShadowMap.SampleLevel(Sampler, D + Offset / Softness, 0).x);
    }
    
    return lerp(Result / Iterations, 1.0, Penumbra);
}

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

	float4 H = mul(float4(Frag.Position, 1), LightViewProjection);
	float2 T = float2(H.x / H.w / 2.0 + 0.5f, 1 - (H.y / H.w / 2.0 + 0.5f));
	[branch] if (H.z > 0.0 && saturate(T.x) == T.x && saturate(T.y) == T.y)
        A *= GetLightness(T, H.z / H.w - Bias) + length(S) / 3.0;
    
	return float4(Lighting * (R + S) * A, A);
};