#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/multisample.hlsl"
#include "std/core/lighting.hlsl"
#include "std/core/atmosphere.hlsl"
#include "std/core/material.hlsl"
#include "std/core/position.hlsl"
#include "lighting/line/common/buffer.hlsl"
#pragma warning(disable: 3595)

Texture2D ShadowMap[6] : register(t5);
SamplerState DepthSampler : register(s1);
SamplerComparisonState DepthLessSampler : register(s2);
SamplerComparisonState DepthGreaterSampler : register(s3);

float GetPenumbra(uniform uint I, float2 D, float L)
{
	[branch] if (Umbra <= 0.0)
		return 0.0;
	
	float Length = 0.0, Count = 0.0;
	[unroll] for (float i = 0; i < 16; i++)
	{
        float2 TexCoord = D + SampleDisk[i].xy / Softness;
		float S1 = ShadowMap[I].SampleLevel(DepthSampler, TexCoord, 0).x;
		float S2 = ShadowMap[I].SampleCmpLevelZero(DepthGreaterSampler, TexCoord, L);
		Length += S1 * S2;
		Count += S2;
	}

	[branch] if (Count < 2.0)
		return 1.0;
	
	Length /= Count;
	return saturate(Umbra * vb_Far * (L - Length) / Length);
}
float GetLightness(uniform uint I, float2 D, float L)
{
	float Penumbra = GetPenumbra(I, D, L);
	[branch] if (Penumbra < 0.0)
		return 1.0;

	float Result = 0.0;
	[loop] for (float j = 0; j < Iterations; j++)
	{
		float2 Offset = SampleDisk[j % 64].xy * (64.0 / max(64.0, j)) / Softness;
		Result += ShadowMap[I].SampleCmpLevelZero(DepthLessSampler, D + Offset, L);
    }

	return lerp(Result / Iterations, 1.0, Penumbra);
}
float GetCascade(float3 Position, uniform uint Index)
{
	[branch] if (Index >= (uint)Cascades)
		return 1.0;

	float4 L = mul(float4(Position, 1), LightViewProjection[Index]);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	[branch] if (saturate(T.x) != T.x || saturate(T.y) != T.y)
		return -1.0;
	
	float D = L.z / L.w - Bias, C, B;
	return GetLightness(Index, T, D);
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	float2 TexCoord = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(TexCoord);
	
	[branch] if (Frag.Depth >= 1.0)
	{
		[branch] if (ScatterIntensity <= 0.0)
			return float4(0, 0, 0, 0);
			
		Scatter A;
		A.Sun = ScatterIntensity;
		A.Planet = PlanetRadius;
		A.Atmos = AtmosphereRadius;
		A.Rlh =  RlhEmission;
		A.Mie = MieEmission;
		A.RlhHeight = RlhHeight;
		A.MieHeight = MieHeight;
		A.MieG = MieDirection;
		return float4(GetAtmosphere(TexCoord, SkyOffset, float3(0, 6372e3, 0), Position, A), 1.0);
	}
	
	Material Mat = Materials[Frag.Material];
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
	float3 E = GetSurface(Frag, Mat);
	float3 D = normalize(vb_Position - Frag.Position);
	float3 R = GetCookTorranceBRDF(Frag.Normal, D, Position, Frag.Diffuse, M, G);
	float3 S = GetSubsurface(Frag.Normal, D, Position, Mat.Scatter) * E;
	R = Lighting * (R + S);

	float H = GetCascade(Frag.Position, 0);
	[branch] if (H >= 0.0)
		return float4(R * (H + S), 1.0);
		
	H = GetCascade(Frag.Position, 1);
	[branch] if (H >= 0.0)
		return float4(R * (H + S), 1.0);
		
	H = GetCascade(Frag.Position, 2);
	[branch] if (H >= 0.0)
		return float4(R * (H + S), 1.0);
		
	H = GetCascade(Frag.Position, 3);
	[branch] if (H >= 0.0)
		return float4(R * (H + S), 1.0);
		
	H = GetCascade(Frag.Position, 4);
	[branch] if (H >= 0.0)
		return float4(R * (H + S), 1.0);
		
	H = GetCascade(Frag.Position, 5);
	[branch] if (H >= 0.0)
		return float4(R * (H + S), 1.0);
	
	return float4(R, 1.0);
};