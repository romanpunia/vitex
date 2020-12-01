#include "renderer/vertex/shape"
#include "workflow/pass"
#include "standard/cook-torrance"

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
	float Recount;
	float Bias;
	float Iterations;
	float Padding;
};

Texture2D ShadowMap : register(t5);
SamplerState ShadowSampler : register(s1);

void SampleShadow(float2 D, float L, out float C, out float B)
{
    C = B = 0.0;
	[loop] for (int x = -Iterations; x < Iterations; x++)
	{
		[loop] for (int y = -Iterations; y < Iterations; y++)
		{
			float2 Shadow = ShadowMap.SampleLevel(ShadowSampler, saturate(D + float2(x, y) / Softness), 0).xy;
			C += step(L, Shadow.x); B += Shadow.y;
		}
	}

	C /= Recount; B /= Recount;
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

	float4 L = mul(float4(Frag.Position, 1), LightViewProjection);
	float2 T = float2(L.x / L.w / 2.0 + 0.5f, 1 - (L.y / L.w / 2.0 + 0.5f));
	[branch] if (L.z > 0.0 && saturate(T.x) == T.x && saturate(T.y) == T.y)
    {
	    float I = L.z / L.w - Bias, C, B;
        [branch] if (Softness <= 0.0)
        {
            float2 Shadow = ShadowMap.SampleLevel(ShadowSampler, T, 0).xy;
            C = step(I, Shadow.x); B = Shadow.y;
        }
        else
            SampleShadow(T, I, C, B);

        A *= C + B * (1.0 - C);
    }

    A *= 1.0 - (1.0 - F) * 1.0 / (1.0 - Cutoff);
	return float4(Lighting * (Frag.Diffuse * E + O) * A, length(A) / 3.0);
};