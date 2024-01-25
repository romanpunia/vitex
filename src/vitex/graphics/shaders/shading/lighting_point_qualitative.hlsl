#include "internal/layouts_shape.hlsl"
#include "internal/channels_effect.hlsl"
#include "internal/utils_lighting.hlsl"
#include "internal/utils_material.hlsl"
#include "internal/utils_position.hlsl"
#include "internal/utils_multisample.hlsl"
#include "shading/internal/lighting_point_buffer.hlsl"
#pragma warning(disable: 4000)

TextureCube DepthMap : register(t5);
TextureCube DepthMapLess : register(t6);
TextureCube DepthMapGreater : register(t7);
SamplerState DepthSampler : register(s5);
SamplerComparisonState DepthLessSampler : register(s6);
SamplerComparisonState DepthGreaterSampler : register(s7);

float GetPenumbra(float3 D, float L)
{
	[branch] if (Umbra <= 0.0)
		return 0.0;
	
	float Length = 0.0;
	[unroll] for (float i = 0; i < 16; i++)
	{
		float3 TexCoord = D + SampleDisk[i] * (i / 64.0) / Softness;
		float S1 = DepthMap.SampleLevel(DepthSampler, TexCoord, 0).x;
		float S2 = DepthMapGreater.SampleCmpLevelZero(DepthGreaterSampler, TexCoord, L);
		Length += S1 * S2;
	}

	Length /= 16.0;
	return saturate(Umbra * (L - Length) / Length);
}
float GetLightness(float3 D, float L)
{
	float Penumbra = GetPenumbra(D, L);
	[branch] if (Penumbra >= 1.0)
		return 1.0;

	float Result = 0.0;
	[loop] for (float j = 0; j < Iterations; j++)
	{
		float3 Offset = SampleDisk[j % 64] * (j / 64.0) / Softness;
		Result += DepthMapLess.SampleCmpLevelZero(DepthLessSampler, D + Offset, L);
	}

	return lerp(Result / Iterations, 1.0, Penumbra);
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), LightTransform);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
	[branch] if (Frag.Depth >= 1.0)
		return float4(0, 0, 0, 0);

	Material Mat = Materials[Frag.Material];
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
	float3 E = GetSurface(Frag, Mat);
	float3 K = Position - Frag.Position;
	float3 D = normalize(vb_Position - Frag.Position);
	float3 L = normalize(K);
	float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
	float3 S = GetSubsurface(Frag.Normal, D, L, Mat.Scatter) * E;
	float A = GetRangeAttenuation(K, Attenuation.x, Attenuation.y, Range);
#ifndef TARGET_D3D
	L.y = -L.y;
#endif
	A *= GetLightness(-L, length(K) / Distance - Bias) + length(S) / 3.0;
	
	return float4(Lighting * (R + S) * A, A);
};