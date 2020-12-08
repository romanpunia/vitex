#include "sdk/buffers/viewer"
#include "sdk/objects/material"
#include "sdk/objects/fragment"

StructuredBuffer<Material> Materials : register(t0);
Texture2D Channel0 : register(t1);
Texture2D Channel1 : register(t2);
Texture2D Channel2 : register(t3);
Texture2D Channel3 : register(t4);
SamplerState Sampler : register(s0);

float GetDepth(float2 TexCoord)
{
    return Channel2.SampleLevel(Sampler, TexCoord, 0).x;
}
float3 GetNormal(float2 TexCoord)
{
    return Channel1.SampleLevel(Sampler, TexCoord, 0).xyz;
}
float4 GetDiffuse(float2 TexCoord, float Level)
{
    return Channel0.SampleLevel(Sampler, TexCoord, Level);
}
Fragment GetFragment(float2 TexCoord)
{
    float4 C0 = Channel0.SampleLevel(Sampler, TexCoord, 0);
    float4 C1 = Channel1.SampleLevel(Sampler, TexCoord, 0);
    float4 C2 = Channel2.SampleLevel(Sampler, TexCoord, 0);
    float4 C3 = Channel3.SampleLevel(Sampler, TexCoord, 0);
    float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, C2.x, 1.0), InvViewProjection);

    Fragment Result;
    Result.Position = Position.xyz / Position.w;
    Result.Diffuse = C0.xyz;
    Result.Alpha = C0.w;
    Result.Normal = C1.xyz;
    Result.Material = C1.w;
    Result.Depth = C2.x;
    Result.Roughness = C3.x;
    Result.Metallic = C3.y;
    Result.Occlusion = C3.z;
    Result.Emission = C3.w;

    return Result;
}
Material GetMaterial(float Material_Id)
{
    return Materials[Material_Id];
}