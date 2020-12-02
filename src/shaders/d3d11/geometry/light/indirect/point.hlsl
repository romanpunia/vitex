#include "renderer/vertex/shape"
#include "workflow/pass"
#include "standard/cook-torrance"
#include "standard/random-float"
#pragma warning(disable: 4000)

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

TextureCube ShadowMap : register(t5);
SamplerState ShadowSampler : register(s1);

float GetPenumbra(in float3 D, in float L)
{
    [branch] if (Umbra <= 0.0)
        return 1.0;
    
    float Length = 0.0, Count = 0.0;
    [unroll] for (int i = 0; i < 16; i++)
    {
        float2 Offset = FiboDisk[i];
        float R = ShadowMap.SampleLevel(ShadowSampler, D + float3(Offset, -Offset.x) / Softness, 0).x;
        float Step = 1.0 - step(L, R);
        Length += R * Step;
        Count += Step;
    }

    [branch] if (Count < 2.0)
        return -1.0;
    
    Length /= Count;
    return max(0.05, saturate(Umbra * (L - Length) / Length));
}
float GetLightness(in float3 D, in float L)
{
    float Penumbra = GetPenumbra(D, L);
    [branch] if (Penumbra < 0.0)
        return 1.0;

    float Result = 0.0, Inter = 0.0;
	[loop] for (int j = 0; j < Iterations; j++)
	{
        float2 Offset = FiboDisk[j];
        float2 R = ShadowMap.SampleLevel(ShadowSampler, D + Penumbra * float3(Offset, -Offset.x) / Softness, 0).xy;
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

	Material Mat = GetMaterial(Frag.Material);
	float3 D = Position - Frag.Position;
	float3 K = normalize(D);
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float L = length(D);
	float A = 1.0 - L / Range;
	float3 P = normalize(ViewPosition - Frag.Position), O;
	float3 E = GetLight(P, K, Frag.Normal, M, R, O);

    E = Lighting * (Frag.Diffuse * E + O);
	return float4(E * GetLightness(-K, L / Distance - Bias) * A, A);
};