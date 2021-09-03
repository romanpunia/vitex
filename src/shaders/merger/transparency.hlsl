#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"
#include "std/core/position.hlsl"
#include "std/core/sampler.hlsl"
#pragma warning(disable: 4000)

cbuffer RenderConstant : register(b3)
{
	float3 Padding;
	float Mips;
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
    float3 Eye = normalize(Position - vb_Position);
    float4 Normal = GetSample(NormalBuffer, TexCoord);

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

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
    float2 TexCoord = GetTexCoord(V.TexCoord);
    float D1 = GetSampleLevel(LDepthBuffer, TexCoord, 0).x;
    float D2 = GetDepth(TexCoord);

    [branch] if (D2 < D1 || D1 >= 1.0)
        return float4(GetOpaque(TexCoord, D2, 0.0), 1.0);

    float3 Position = GetPosition(TexCoord, D1);
    float3 Eye = normalize(Position - vb_Position);
    float4 Normal = GetSample(LNormalBuffer, TexCoord);
    
    Material Mat = Materials[Normal.w];
    float4 Diffuse = GetSample(LDiffuseBuffer, TexCoord);
    float A = (1.0 - Diffuse.w) * Mat.Transparency;
    float R = max(0.0, Mat.Roughness.x + GetSample(LSurfaceBuffer, TexCoord).x * Mat.Roughness.y - 0.25) / 0.75;

    Diffuse.x += GetCoverage(GetUV(TexCoord, Mat.Refraction, 0.05), R * Mips).x * A;
    Diffuse.y += GetCoverage(GetUV(TexCoord, Mat.Refraction, 0.0), R * Mips).y * A;
    Diffuse.z += GetCoverage(GetUV(TexCoord, Mat.Refraction, -0.05), R * Mips).z * A;

    return float4(Diffuse.xyz, 1.0 - A);
};