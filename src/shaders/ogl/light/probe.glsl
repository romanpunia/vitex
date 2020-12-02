#include "renderer/vertex/shape"
#include "standard/cook-torrance"
#include "workflow/effects"

cbuffer RenderConstant : register(b3)
{
	matrix LightWorldViewProjection;
	float3 Position;
	float Range;
	float3 Lighting;
	float MipLevels;
	float3 Scale;
	float Parallax;
    float3 Padding;
    float Infinity;
};

TextureCube EnvironmentMap : register(t5);

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
    [branch] if (Mat.Environment <= 0.0)
        return float4(0.0, 0.0, 0.0, 0.0);

	float3 D = Position - Frag.Position;
    float3 E = normalize(Frag.Position - ViewPosition);
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float A = max(Infinity, (1.0 - length(D) / Range)) * Mat.Environment;

	D = -normalize(reflect(-E, -Frag.Normal));
	[branch] if (Parallax > 0)
	{
		float3 Max = Position + Scale;
		float3 Min = Position - Scale;
		float3 Plane = ((D > 0.0 ? Max : Min) - Frag.Position) / D;

		D = Frag.Position + D * min(min(Plane.x, Plane.y), Plane.z) - Position;
	}
    
    float T = GetRoughnessLevel(Frag, Mat, MipLevels);
    float3 C = GetReflection(E, normalize(D), Frag.Normal, M, R);
    float3 P = GetSample3Level(EnvironmentMap, D, T).xyz;

    return float4(Lighting * P * A * C, A);
};