#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/lighting.hlsl"
#include "std/core/material.hlsl"
#include "std/core/multisample.hlsl"
#include "std/core/position.hlsl"
#include "lighting/spot/common/buffer.hlsl"
#pragma warning(disable: 4000)

Texture2D DepthMap : register(t5);
Texture2D DepthMapLess : register(t6);
Texture2D DepthMapGreater : register(t7);
SamplerState DepthSampler : register(s5);
SamplerComparisonState DepthLessSampler : register(s6);
SamplerComparisonState DepthGreaterSampler : register(s7);

float GetPenumbra(float2 D, float L)
{
	[branch] if (Umbra <= 0.0)
		return 0.0;
	
	float Length = 0.0;
	[unroll] for (float i = 0; i < 16; i++)
	{
		float2 TexCoord = D + SampleDisk[i].xy / Softness;
		float S1 = DepthMap.SampleLevel(DepthSampler, TexCoord, 0).x;
		float S2 = DepthMapGreater.SampleCmpLevelZero(DepthGreaterSampler, TexCoord, L);
		Length += S1 * S2;
	}
	
	Length /= 16.0;
	return saturate(Umbra * vb_Far * (L - Length) / Length);
}
float GetLightness(float2 D, float L)
{
	float Penumbra = GetPenumbra(D, L);
	[branch] if (Penumbra >= 1.0)
		return 1.0;

	float Result = 0.0;
	[loop] for (float j = 0; j < Iterations; j++)
	{
		float2 Offset = SampleDisk[j % 64].xy * (j / 64.0) / Softness;
		Result += DepthMapLess.SampleCmpLevelZero(DepthLessSampler, D + Offset, L);
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

	Material Mat = Materials[Frag.Material];
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
	float3 E = GetSurface(Frag, Mat);
	float3 D = normalize(vb_Position - Frag.Position);
	float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
	float3 S = GetSubsurface(Frag.Normal, D, L, Mat.Scatter) * E;

	float4 H = mul(float4(Frag.Position, 1), LightViewProjection);
#ifdef TARGET_D3D
	float2 T = float2(H.x / H.w / 2.0 + 0.5f, 1 - (H.y / H.w / 2.0 + 0.5f));
#else
	float2 T = float2(H.x / H.w / 2.0 + 0.5f, H.y / H.w / 2.0 + 0.5f);
#endif
	[branch] if (H.z > 0.0 && saturate(T.x) == T.x && saturate(T.y) == T.y)
		A *= GetLightness(T, H.z / H.w - Bias) + length(S) / 3.0;
	
	return float4(Lighting * (R + S) * A, A);
};