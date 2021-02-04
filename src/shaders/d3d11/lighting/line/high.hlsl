#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/random"
#include "std/core/lighting"
#include "std/core/atmosphere"
#include "std/core/material"
#include "std/core/position"
#include "lighting/line/common/buffer"
#pragma warning(disable: 3595)

Texture2D ShadowMap[6] : register(t5);
SamplerState ShadowSampler : register(s1);

float GetPenumbra(uniform uint I, float2 D, float L)
{
    [branch] if (Umbra <= 0.0)
        return 1.0;
    
    float Length = 0.0, Count = 0.0;
    [unroll] for (int i = 0; i < 16; i++)
    {
        float S1 = ShadowMap[I].SampleLevel(ShadowSampler, D + FiboDisk[i] / Softness, 0).x;
        float S2 = 1.0 - step(L, S1);
        Length += S1 * S2;
        Count += S2;
    }

    [branch] if (Count < 2.0)
        return 1.0;
    
    Length /= Count;
    return max(0.1, saturate(Umbra * FarPlane * (L - Length) / Length));
}
float GetLightness(uniform uint I, float2 D, float L)
{
    float Penumbra = GetPenumbra(I, D, L);
    [branch] if (Penumbra < 0.0)
        return 1.0;

    float Result = 0.0;
	[loop] for (int j = 0; j < Iterations; j++)
        Result += step(L, ShadowMap[I].SampleLevel(ShadowSampler, D + Penumbra * FiboDisk[j] / Softness, 0).x);

    return Result / Iterations;
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

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
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
    
	Material Mat = GetMaterial(Frag.Material);
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 E = GetSurface(Frag, Mat);
    float3 D = normalize(ViewPosition - Frag.Position);
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