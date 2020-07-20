#include "standard/space-sv"
#include "standard/cook-torrance"
#pragma warning(disable: 4000)

cbuffer RenderConstant : register(b3)
{
	float MipLevels;
	float3 Padding;
}

Texture2D LChannel0 : register(t5);
Texture2D LChannel1 : register(t6);
Texture2D LChannel2 : register(t7);
Texture2D LChannel3 : register(t8);

float3 GetOpaque(in float2 TexCoord, in float D2, in float L)
{
    [branch] if (D2 >= 1.0)
        return GetDiffuseLevel(TexCoord, L).xyz;

    float3 Position = GetPosition(TexCoord, D2);
    float3 Eye = normalize(Position - ViewPosition);
    float4 Normal = GetSample(Channel1, TexCoord);
    float Fresnel = GetFresnel(Eye, Normal.xyz);
    Material Mat = GetMaterial(Normal.w);

    return lerp(GetDiffuseLevel(TexCoord, L).xyz, 1.0, Fresnel * Mat.Fresnel);
}
float3 GetOpaqueAuto(in float2 TexCoord, in float L)
{
    float D2 = GetDepth(TexCoord);
    return GetOpaque(TexCoord, D2, L);
}
float2 GetUV(in float2 TexCoord, in float L, in float V)
{
	float2 T = TexCoord - 0.5;
    float R = T.x * T.x + T.y * T.y;
	float F = 1.0 + R * (L + lerp(0, V, abs(L)));
    
    return F * T + 0.5;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float2 TexCoord = GetTexCoord(V.TexCoord);
    float D1 = GetSampleLevel(LChannel2, TexCoord, 0).x;
    float D2 = GetDepth(TexCoord);

    [branch] if (D2 < D1 || D1 >= 1.0)
        return float4(GetOpaque(TexCoord, D2, 0.0), 1.0);

    float3 Position = GetPosition(TexCoord, D1);
    float3 Eye = normalize(Position - ViewPosition);
    float4 Normal = GetSample(LChannel1, TexCoord);
    float Fresnel = GetFresnel(Eye, Normal.xyz);
    Material Mat = GetMaterial(Normal.w);
    float4 Diffuse = float4(GetSample(LChannel0, TexCoord).xyz, max(0, 1.0 - Mat.Limpidity));
    float R = Mat.Roughness.x + GetSample(LChannel3, TexCoord).x * Mat.Roughness.y;

    Diffuse.xyz = lerp(Diffuse.xyz, 1.0, Fresnel * Mat.Fresnel);
    Diffuse.x += GetOpaqueAuto(GetUV(TexCoord, -Mat.Refraction, 0.05), R * MipLevels).x * Mat.Limpidity;
    Diffuse.y += GetOpaqueAuto(GetUV(TexCoord, -Mat.Refraction, 0.0), R * MipLevels).y * Mat.Limpidity;
    Diffuse.z += GetOpaqueAuto(GetUV(TexCoord, -Mat.Refraction, -0.05), R * MipLevels).z * Mat.Limpidity;

    return Diffuse;
};