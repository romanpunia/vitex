#include "standard/space-sv"
#include "standard/cook-torrance"
#include "standard/atmosphere"
#include "standard/random-float"
#pragma warning(disable: 3595)

cbuffer RenderConstant : register(b3)
{
    matrix LightViewProjection[6];
    matrix SkyOffset;
    float3 RlhEmission;
    float RlhHeight;
    float3 MieEmission;
    float MieHeight;
    float3 Lighting;
    float Softness;
    float3 Position;
    float Cascades;
    float Bias;
    float Iterations;
    float ScatterIntensity;
    float PlanetRadius;
    float AtmosphereRadius;
    float MieDirection;
    float Umbra;
    float Padding;
};

Texture2D ShadowMap[6] : register(t5);
SamplerState ShadowSampler : register(s1);

float GetPenumbra(uniform uint I, in float2 D, in float L)
{
    [branch] if (Umbra <= 0.0)
        return 1.0;
    
    float Length = 0.0, Count = 0.0;
    [unroll] for (int i = 0; i < 16; i++)
    {
        float R = ShadowMap[I].SampleLevel(ShadowSampler, D + FiboDisk[i] / Softness, 0).x;
        float Step = 1.0 - step(L, R);
        Length += R * Step;
        Count += Step;
    }

    [branch] if (Count < 2.0)
        return 1.0;
    
    Length /= Count;
    return max(0.05, saturate(Umbra * FarPlane * (L - Length) / Length));
}
float GetLightness(uniform uint I, in float2 D, in float L)
{
    float Penumbra = GetPenumbra(I, D, L);
    [branch] if (Penumbra < 0.0)
        return 1.0;

    float Result = 0.0, Inter = 0.0;
	[loop] for (int j = 0; j < Iterations; j++)
	{
        float2 R = ShadowMap[I].SampleLevel(ShadowSampler, D + Penumbra * FiboDisk[j] / Softness, 0).xy;
        Result += step(L, R.x);
        Inter += R.y;
	}

	Result /= Iterations;
    return Result + (Inter / Iterations) * (1.0 - Result);
}
float SampleCascade(in float3 Position, in float G, uniform uint Index)
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

float4 PS(VertexResult V) : SV_TARGET0
{
    float2 TexCoord = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(TexCoord);

    [branch] if (Frag.Depth >= 1.0)
    {
        [branch] if (ScatterIntensity <= 0.0)
            discard;
            
        Scatter A;
        A.Sun = ScatterIntensity * length(Lighting);
        A.Planet = PlanetRadius;
        A.Atmos = AtmosphereRadius;
        A.Rlh = Lighting * RlhEmission;
        A.Mie = MieEmission;
        A.RlhHeight = RlhHeight;
        A.MieHeight = MieHeight;
        A.MieG = MieDirection;
        return float4(GetAtmosphere(TexCoord, SkyOffset, float3(0, 6372e3, 0), Position, A), 1.0);
    }
    
	Material Mat = GetMaterial(Frag.Material);
	float3 D = Position;
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float3 P = normalize(ViewPosition - Frag.Position), O;
	float3 E = GetLight(P, D, Frag.Normal, M, R, O);
    float G = length(float2(ViewPosition.x - P.x, ViewPosition.z - P.z));
    E = Lighting * (Frag.Diffuse * E + O);

    float S = SampleCascade(Frag.Position, G, 0);
    [branch] if (S >= 0.0)
        return float4(E * S, 1.0);
        
    S = SampleCascade(Frag.Position, G, 1);
    [branch] if (S >= 0.0)
        return float4(E * S, 1.0);
        
    S = SampleCascade(Frag.Position, G, 2);
    [branch] if (S >= 0.0)
        return float4(E * S, 1.0);
        
    S = SampleCascade(Frag.Position, G, 3);
    [branch] if (S >= 0.0)
        return float4(E * S, 1.0);
        
    S = SampleCascade(Frag.Position, G, 4);
    [branch] if (S >= 0.0)
        return float4(E * S, 1.0);
        
    S = SampleCascade(Frag.Position, G, 5);
    [branch] if (S >= 0.0)
        return float4(E * S, 1.0);
    
	return float4(E, 1.0);
};