#include "standard/space-sv"
#include "standard/cook-torrance"
#include "standard/ray-march"

cbuffer RenderConstant : register(b3)
{
	float IterationCount;
	float MipLevels;
	float Intensity;
	float Padding;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
	Material Mat = GetMaterial(Frag.Material);
	float R = GetRoughnessFactor(Frag, Mat);
	float3 M = GetMetallicFactor(Frag, Mat);
	float3 D = reflect(normalize(Frag.Position - ViewPosition.xyz), Frag.Normal);
    float2 Hit;

    [branch] if (!RayMarch(Frag.Position, D, IterationCount, Hit))
        return float4(Frag.Diffuse, 1.0);

    float3 E = normalize(Frag.Position - ViewPosition.xyz);
    float3 C = GetReflection(E, normalize(D), Frag.Normal, M, R);
    float3 L = GetDiffuseLevel(Hit, MipLevels * R).xyz;
    float A = RayEdgeSample(Hit);
    
	return float4(Frag.Diffuse + L * C * A, 1.0);
};