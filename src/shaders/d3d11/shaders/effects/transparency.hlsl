#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/position"
#include "sdk/core/sampler"
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

float3 GetOpaque(float2 TexCoord, float D2, float L)
{
    [branch] if (D2 >= 1.0)
        return GetDiffuse(TexCoord, L).xyz;

    float3 Position = GetPosition(TexCoord, D2);
    float3 Eye = normalize(Position - ViewPosition);
    float4 Normal = GetSample(Channel1, TexCoord);
    Material Mat = GetMaterial(Normal.w);

    return GetDiffuse(TexCoord, L).xyz;
}
float3 GetOpaqueAuto(float2 TexCoord, float L)
{
    float D2 = GetDepth(TexCoord);
    return GetOpaque(TexCoord, D2, L);
}
float2 GetUV(float2 TexCoord, float L, float V)
{
	float2 T = TexCoord - 0.5;
    float R = T.x * T.x + T.y * T.y;
	float F = 1.0 + R * (L + lerp(0, V, abs(L)));
    
    return F * T + 0.5;
}

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = V.Position;
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    float2 TexCoord = GetTexCoord(V.TexCoord);
    float D1 = GetSampleLevel(LChannel2, TexCoord, 0).x;
    float D2 = GetDepth(TexCoord);

    [branch] if (D2 < D1 || D1 >= 1.0)
        return float4(GetOpaque(TexCoord, D2, 0.0), 1.0);

    float3 Position = GetPosition(TexCoord, D1);
    float3 Eye = normalize(Position - ViewPosition);
    float4 Normal = GetSample(LChannel1, TexCoord);
    Material Mat = GetMaterial(Normal.w);
    float4 Diffuse = float4(GetSample(LChannel0, TexCoord).xyz, max(0, 1.0 - Mat.Transparency));
    float R = Mat.Roughness.x + GetSample(LChannel3, TexCoord).x * Mat.Roughness.y;

    Diffuse.x += GetOpaqueAuto(GetUV(TexCoord, -Mat.Refraction, 0.05), R * MipLevels).x * Mat.Transparency;
    Diffuse.y += GetOpaqueAuto(GetUV(TexCoord, -Mat.Refraction, 0.0), R * MipLevels).y * Mat.Transparency;
    Diffuse.z += GetOpaqueAuto(GetUV(TexCoord, -Mat.Refraction, -0.05), R * MipLevels).z * Mat.Transparency;

    return Diffuse;
};