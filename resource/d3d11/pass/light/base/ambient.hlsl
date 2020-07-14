#include "renderer/vertex/shape"
#include "workflow/pass"

Texture2D LightMap : register(t4);
TextureCube SkyMap : register(t5);

cbuffer RenderConstant : register(b3)
{
	float3 HighEmission;
	float SkyEmission;
	float3 LowEmission;
	float Padding;
    matrix SkyOffset;
};

struct AmbientVertexResult
{
    float4 Position : SV_POSITION;
    float4 TexCoord : TEXCOORD0;
    float3 View : TEXCOORD1;
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
    
    float4 View = mul(float4(V.Position.xy, 1.0, 1.0), SkyOffset);
    Result.View = View.xyz / View.w;

	return Result;
}

float4 PS(AmbientVertexResult V) : SV_TARGET0
{
    Fragment Frag = GetFragment(V.TexCoord.xy);
    Material Mat = GetMaterial(Frag.Material);
    float3 Emission = Mat.Emission.xyz * Mat.Emission.w;
	float4 Light = GetSample(LightMap, V.TexCoord.xy);
	float3 Ambient = GetHemiAmbient(HighEmission, LowEmission, Frag.Normal.y);
    float3 Sky = float3(0, 0, 0);
    Emission = GetHemiAmbient(Emission, Emission * 0.5, Frag.Normal.y);
    
    [branch] if (Frag.Depth >= 1.0)
        Sky = SkyMap.Sample(Sampler, V.View).xyz * SkyEmission;

	return float4(Frag.Diffuse * (Light.xyz + Ambient + Emission) + (Emission + Frag.Diffuse) * Light.xyz * Light.a + Sky, 1.0);
};