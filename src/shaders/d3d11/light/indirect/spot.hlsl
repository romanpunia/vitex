#include "renderer/vertex/shape"
#include "workflow/effects"
#include "standard/cook-torrance"
#include "standard/random-float"
#pragma warning(disable: 4000)

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

Texture2D ShadowMap : register(t5);
SamplerState ShadowSampler : register(s1);

float GetPenumbra(in float2 D, in float L)
{
    [branch] if (Umbra <= 0.0)
        return 1.0;
    
    float Length = 0.0, Count = 0.0;
    [unroll] for (int i = 0; i < 16; i++)
    {
        float R = ShadowMap.SampleLevel(ShadowSampler, D + FiboDisk[i] / Softness, 0).x;
        float Step = 1.0 - step(L, R);
        Length += R * Step;
        Count += Step;
    }

    [branch] if (Count < 2.0)
        return -1.0;
    
    Length /= Count;
    return max(0.05, saturate(Umbra * FarPlane * (L - Length) / Length));
}
float GetLightness(in float2 D, in float L)
{
    float Penumbra = GetPenumbra(D, L);
    [branch] if (Penumbra < 0.0)
        return 1.0;

    float Result = 0.0, Inter = 0.0;
	[loop] for (int j = 0; j < Iterations; j++)
	{
        float2 R = ShadowMap.SampleLevel(ShadowSampler, D + Penumbra * FiboDisk[j] / Softness, 0).xy;
        Result += step(L, R.x);
        Inter += R.y;
	}

	Result /= Iterations;
    return Result + (Inter / Iterations) * (1.0 - Result);
}

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, LightWorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

	float3 D = Position - Frag.Position;
    float3 K = normalize(D);
    float F = dot(-K, Direction);
    [branch] if (F <= Cutoff)
        return float4(0, 0, 0, 0);

	Material Mat = GetMaterial(Frag.Material);
	float3 A = 1.0 - length(D) / Range, O;
	float3 P = normalize(ViewPosition - Frag.Position);
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
    float3 E = GetLight(P, K, Frag.Normal, M, R, O);
    float3 H = Mat.Emission.xyz * GetThickness(P, K, Frag.Normal, Mat.Scatter);

	float4 L = mul(float4(Frag.Position, 1), LightViewProjection);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	[branch] if (L.z > 0.0 && saturate(T.x) == T.x && saturate(T.y) == T.y)
        A *= GetLightness(T, L.z / L.w - Bias) + H;
    
    A *= 1.0 - (1.0 - F) * 1.0 / (1.0 - Cutoff);
	return float4(Lighting * (Frag.Diffuse * (E + H) + O) * A, length(A) / 3.0);
};