#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/lighting"
#include "sdk/core/material"
#include "sdk/core/sampler"
#include "sdk/core/position"

cbuffer RenderConstant : register(b3)
{
	matrix LightWorldViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float MipLevels;
	float3 Scale;
	float Parallax;
    float3 Attenuation;
    float Infinity;
};

TextureCube EnvironmentMap : register(t5);

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), LightWorldViewProjection);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);

	Material Mat = GetMaterial(Frag.Material);
    [branch] if (Mat.Environment <= 0.0)
        return float4(0.0, 0.0, 0.0, 0.0);

	float3 D = Position - Frag.Position;
    float3 E = normalize(Frag.Position - ViewPosition);
	float3 M = GetMetallic(Frag, Mat);
	float R = GetRoughness(Frag, Mat);
	float A = max(Infinity, GetRangeAttenuation(D, Attenuation.x, Attenuation.y, Range)) * Mat.Environment;

	D = -normalize(reflect(-E, -Frag.Normal));
	[branch] if (Parallax > 0)
	{
		float3 Max = Position + Scale;
		float3 Min = Position - Scale;
		float3 Plane = ((D > 0.0 ? Max : Min) - Frag.Position) / D;

		D = Frag.Position + D * min(min(Plane.x, Plane.y), Plane.z) - Position;
	}
    
    float T = GetRoughnessMip(Frag, Mat, MipLevels);
    float3 P = GetSample3Level(EnvironmentMap, D, T).xyz;
    float3 C = GetSpecularBRDF(Frag.Normal, -E, normalize(D), P, M, R);

    return float4(Lighting * C * A, A);
};