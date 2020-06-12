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
	float3 Color = GetDiffuse(V.TexCoord.xy).xyz;
	float4 Light = GetSample(LightMap, V.TexCoord.xy);
	float3 Normal = GetNormal(V.TexCoord.xy);
	float3 Ambient = HemiAmbient(HighEmission, LowEmission, Normal.y);

	return float4(Color.xyz * (Light.xyz + Ambient) + Light.xyz * Light.a, 1.0f);
};