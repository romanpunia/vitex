#include "standard/space-sv"
#include "standard/cook-torrance"
#include "standard/atmosphere"

cbuffer RenderConstant : register(b3)
{
    matrix LightViewProjection[6];
    matrix SkyOffset;
    float3 RlhEmission;
    float RlhHeight;
    float3 MieEmission;
    float MieHeight;
    float3 Lighting;
    float Softness;
    float3 Position;
    float Recount;
    float Cascades;
    float Bias;
    float Iterations;
    float ScatterIntensity;
    float PlanetRadius;
    float AtmosphereRadius;
    float MieDirection;
    float Padding;
};

float4 PS(VertexResult V) : SV_TARGET0
{
    float2 TexCoord = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(TexCoord);

    [branch] if (Frag.Depth >= 1.0)
    {
        [branch] if (ScatterIntensity <= 0.0)
            discard;
            
        Scatter A;
        A.Sun = ScatterIntensity * length(Lighting);
        A.Planet = PlanetRadius;
        A.Atmos = AtmosphereRadius;
        A.Rlh = Lighting * RlhEmission;
        A.Mie = MieEmission;
        A.RlhHeight = RlhHeight;
        A.MieHeight = MieHeight;
        A.MieG = MieDirection;
        return float4(GetAtmosphere(TexCoord, SkyOffset, float3(0, 6372e3, 0), Position, A), 1.0);
    }

	Material Mat = GetMaterial(Frag.Material);
	float3 M = GetMetallicFactor(Frag, Mat);
	float R = GetRoughnessFactor(Frag, Mat);
	float3 P = normalize(ViewPosition - Frag.Position);
	float3 D = Position, O;
    float3 E = GetLight(P, D, Frag.Normal, M, R, O);

	return float4(Lighting * (Frag.Diffuse * E + O), 1.0);
};