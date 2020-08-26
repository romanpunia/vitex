#include "renderer/vertex/shape"
#include "workflow/pass"
#include "standard/cook-torrance"

Texture2D LightMap : register(t5);
TextureCube SkyMap : register(t6);

cbuffer RenderConstant : register(b3)
{
    matrix SkyOffset;
	float3 HighEmission;
	float SkyEmission;
	float3 LowEmission;
	float LightEmission;
    float3 SkyColor;
	float Padding;
};

struct AmbientVertexResult
{
    float4 Position : SV_POSITION;
    float4 TexCoord : TEXCOORD0;
    float4 View : TEXCOORD1;
};

float3 GetHemiAmbient(in float3 Up, in float3 Down, in float Height)
{
    float Intensity = Height * 0.5f + 0.5f;
    return Intensity * Up + Down;
}

AmbientVertexResult VS(VertexBase V)
{
	AmbientVertexResult Result = (AmbientVertexResult)0;
	Result.Position = V.Position;
	Result.TexCoord.xy = V.TexCoord;
    Result.View = mul(float4(V.Position.xy, 1.0, 1.0), SkyOffset);
    Result.View.xyz /= Result.View.w;

	return Result;
}

float4 PS(AmbientVertexResult V) : SV_TARGET0
{
    Fragment Frag = GetFragment(V.TexCoord.xy);
	float4 Light = GetSample(LightMap, V.TexCoord.xy) * LightEmission;

    [branch] if (Frag.Depth >= 1.0)
        return float4(Frag.Diffuse + Light.xyz * SkyColor * (1.0 - SkyEmission) + SkyMap.Sample(Sampler, V.View.xyz).xyz * SkyEmission, 1.0);

    Material Mat = GetMaterial(Frag.Material);
    float3 Emission = GetEmissionFactor(Frag, Mat);
	float3 Ambient = GetHemiAmbient(HighEmission, LowEmission, Frag.Normal.y);
    float Fresnel = GetFresnel(normalize(Frag.Position - ViewPosition), Frag.Normal);

    Emission = GetHemiAmbient(Emission, Emission * 0.5, Frag.Normal.y);
    Frag.Diffuse = lerp(Frag.Diffuse.xyz, 1.0, Fresnel * Mat.Fresnel);

	return float4(Frag.Diffuse * (Light.xyz + Ambient + Emission) + (Emission + Frag.Diffuse) * Light.xyz * Light.a, 1.0);
};