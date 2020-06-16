#include "standard/space-uv"
#include "standard/hemi-ambient"

Texture2D LightMap : register(t4);

cbuffer RenderConstant : register(b3)
{
	float3 HighEmission;
	float Padding1;
	float3 LowEmission;
	float Padding2;
};

float4 PS(VertexResult V) : SV_TARGET0
{
    Fragment Frag = GetFragment(V.TexCoord.xy);
    Material Mat = GetMaterial(Frag.Material);
    float3 Emission = Frag.Diffuse * Mat.Emission.xyz * Mat.Emission.w;
	float4 Light = GetSample(LightMap, V.TexCoord.xy);
	float3 Ambient = HemiAmbient(HighEmission, LowEmission, Frag.Normal.y);
    Emission = HemiAmbient(Emission, Emission * 0.5, Frag.Normal.y);
    
	return float4(Frag.Diffuse * (Light.xyz + Ambient + Emission) + (Emission + 1.0) * Light.xyz * Light.a, 1.0);
};