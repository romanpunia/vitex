#include "standard/space-sv"
#include "standard/cook-torrance"
#include "standard/ray-march"

cbuffer RenderConstant : register(b3)
{
	float Samples;
	float MipLevels;
	float Intensity;
	float Padding;
}

float4 PS(VertexResult V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0.0, 0.0, 0.0, 1.0);

	Material Mat = GetMaterial(Frag.Material);
	float3 E = normalize(Frag.Position - ViewPosition);
	float3 D = reflect(E, Frag.Normal);
    float3 HitCoord;

    [branch] if (!RayMarch(Frag.Position, D, Samples, HitCoord))
        return float4(0.0, 0.0, 0.0, 1.0);

    float T = GetRoughnessLevel(Frag, Mat, 1.0);
    float R = GetRoughnessFactor(Frag, Mat);
	float3 M = GetMetallicFactor(Frag, Mat);
    float3 C = GetReflection(E, normalize(D), Frag.Normal, M, R);
    float3 L = GetDiffuseLevel(HitCoord.xy, T * MipLevels).xyz * Intensity;
    float P = length(GetPosition(HitCoord.xy, HitCoord.z) - Frag.Position) * T;
    float A = RayEdgeSample(HitCoord.xy) / (1.0 + 6.0 * P * P);
    
	return float4(L * C * A, 1.0);
};