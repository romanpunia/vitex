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

Texture2D LDiffuseBuffer : register(t5);
Texture2D LNormalBuffer : register(t6);
Texture2D LDepthBuffer : register(t7);
Texture2D LSurfaceBuffer : register(t8);

float3 GetOpaque(float2 TexCoord, float D2, float L)
{
    [branch] if (D2 >= 1.0)
        return GetDiffuse(TexCoord, L).xyz;

    float3 Position = GetPosition(TexCoord, D2);
    float3 Eye = normalize(Position - ViewPosition);
    float4 Normal = GetSample(NormalBuffer, TexCoord);
    Material Mat = GetMaterial(Normal.w);

    return GetDiffuse(TexCoord, L).xyz;
}
float3 GetCoverage(float2 TexCoord, float L)
{
    return GetOpaque(TexCoord, GetDepth(TexCoord), L);
}
float2 GetUV(float2 TexCoord, float L, float V)
{
	float2 T = TexCoord - 0.5;
    float R = T.x * T.x + T.y * T.y;
	float F = 1.0 + R * (lerp(0, V, abs(L)) - L);
    
    return F * T + 0.5;
}

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    float2 TexCoord = GetTexCoord(V.TexCoord);
    float D1 = GetSampleLevel(LDepthBuffer, TexCoord, 0).x;
    float D2 = GetDepth(TexCoord);

    [branch] if (D2 < D1 || D1 >= 1.0)
        return float4(GetOpaque(TexCoord, D2, 0.0), 1.0);

    float3 Position = GetPosition(TexCoord, D1);
    float3 Eye = normalize(Position - ViewPosition);
    float4 Normal = GetSample(LNormalBuffer, TexCoord);
    Material Mat = GetMaterial(Normal.w);
    float4 Diffuse = GetSample(LDiffuseBuffer, TexCoord);
    float A = (1.0 - Diffuse.w) * Mat.Transparency;
    float R = max(0.0, Mat.Roughness.x + GetSample(LSurfaceBuffer, TexCoord).x * Mat.Roughness.y - 0.25) / 0.75;

    Diffuse.x += GetCoverage(GetUV(TexCoord, Mat.Refraction, 0.05), R * MipLevels).x * A;
    Diffuse.y += GetCoverage(GetUV(TexCoord, Mat.Refraction, 0.0), R * MipLevels).y * A;
    Diffuse.z += GetCoverage(GetUV(TexCoord, Mat.Refraction, -0.05), R * MipLevels).z * A;

    return float4(Diffuse.xyz, 1.0 - A);
};