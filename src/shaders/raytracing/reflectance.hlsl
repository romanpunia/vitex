#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/lighting"
#include "std/core/raymarching"
#include "std/core/material"
#include "std/core/position"

cbuffer RenderConstant : register(b3)
{
	float Samples;
	float MipLevels;
	float Intensity;
	float Distance;
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0.0, 0.0, 0.0, 1.0);

    float3 E = normalize(Frag.Position - ViewPosition);
	float3 D = reflect(E, Frag.Normal);
    float A = Rayprefix(E, D);
    [branch] if (A <= 0.0)
        return float4(0.0, 0.0, 0.0, 1.0);

    float3 TexCoord = Raymarch(Frag.Position, D, Samples, Distance);
    [branch] if (TexCoord.z < 0.0)
        return float4(0.0, 0.0, 0.0, 1.0);

	Material Mat = GetMaterial(Frag.Material);
    float T = GetRoughnessMip(Frag, Mat, 1.0);
    float R = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float G = Rayreduce(Frag.Position, TexCoord, T);
    float3 L = GetDiffuse(TexCoord.xy, (1.0 - G) * T * MipLevels).xyz * Intensity;
    float3 C = GetSpecularBRDF(Frag.Normal, -E, normalize(D), L, M, R); 
    A *= Raypostfix(TexCoord.xy, D) * pow(abs(G), 0.8);

	return float4(C * A, 1.0);
};