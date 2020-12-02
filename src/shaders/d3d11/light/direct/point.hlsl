#include "renderer/vertex/shape"
#include "workflow/effects"
#include "standard/cook-torrance"

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
	float A = 1.0 - length(D) / Range;
	float3 P = normalize(ViewPosition - Frag.Position), O;
    float3 E = GetLight(P, K, Frag.Normal, M, R, O);
    E += Mat.Emission.xyz * GetThickness(P, K, Frag.Normal, Mat.Scatter);

	return float4(Lighting * (Frag.Diffuse * E + O) * A, A);
};