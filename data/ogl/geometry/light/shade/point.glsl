#include "renderer/vertex/shape"
#include "workflow/pass"
#include "standard/cook-torrance"

cbuffer RenderConstant : register(b3)
{
	matrix OwnWorldViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float Distance;
	float Softness;
	float Recount;
	float Bias;
	float Iterations;
};

TextureCube ShadowMap : register(t5);

void SampleShadow(in float3 D, in float L, out float C, out float B)
{
	[loop] for (int x = -Iterations; x < Iterations; x++)
	{
		[loop] for (int y = -Iterations; y < Iterations; y++)
		{
			[loop] for (int z = -Iterations; z < Iterations; z++)
			{
				float2 Shadow = GetSample3Level(ShadowMap, D + float3(x, y, z) / Softness, 0).xy;
				C += step(L, Shadow.x); B += Shadow.y;
			}
		}
	}

	C /= Recount; B /= Recount;
}

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
	Result.Position = mul(V.Position, OwnWorldViewProjection);
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
	float I = L / Distance - Bias, C = 0.0, B = 0.0;
	float3 E = GetLight(P, K, Frag.Normal, M, R, O);

	[branch] if (Softness <= 0.0)
	{
		float2 Shadow = GetSample3Level(ShadowMap, -K, 0).xy;
		C = step(I, Shadow.x); B = Shadow.y;
	}
	else
		SampleShadow(-K, I, C, B);

    E = Lighting * (Frag.Diffuse * E + O);
	E *= C + B * (1.0 - C);
	return float4(E * A, A);
};