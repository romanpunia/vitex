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
    [branch] if (Frag.Depth >= 1.0)
        return float4(Frag.Diffuse, 1.0);

	Material Mat = GetMaterial(Frag.Material);
	float3 D = reflect(normalize(Frag.Position - ViewPosition.xyz), Frag.Normal);
    float3 HitCoord;

    [branch] if (!RayMarch(Frag.Position, D, IterationCount, HitCoord))
        return float4(Frag.Diffuse, 1.0);

    float R = GetRoughnessFactor(Frag, Mat);
	float3 M = GetMetallicFactor(Frag, Mat);
	float3 E = normalize(Frag.Position - ViewPosition.xyz);
    float3 C = GetReflection(E, normalize(D), Frag.Normal, M, R);
    float3 L = GetDiffuseLevel(HitCoord.xy, MipLevels * R).xyz * Intensity;
    float A = RayEdgeSample(HitCoord.xy);
    
	return float4(Frag.Diffuse + L * C * A, 1.0);
};