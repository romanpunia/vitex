#include "renderer/vertex/shape"
#include "workflow/effects"
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
	float Bias;
	float Iterations;
	float Umbra;
    float Padding;
};

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

    E += Mat.Emission.xyz * GetThickness(P, K, Frag.Normal, Mat.Scatter);
    A *= 1.0 - (1.0 - F) * 1.0 / (1.0 - Cutoff);

	return float4(Lighting * (Frag.Diffuse * E + O) * A, length(A) / 3.0);
};