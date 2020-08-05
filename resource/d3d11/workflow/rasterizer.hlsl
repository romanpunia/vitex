#include "renderer/input/base"
#include "renderer/buffer/viewer"

float GetRoughness(in float2 TexCoord)
{
    return RoughnessMap.Sample(Sampler, TexCoord).x;
}
float GetMetallic(in float2 TexCoord)
{
    return MetallicMap.Sample(Sampler, TexCoord).x;
}
float GetOcclusion(in float2 TexCoord)
{
    return OcclusionMap.Sample(Sampler, TexCoord).x;
}
float GetEmission(in float2 TexCoord)
{
    return EmissionMap.Sample(Sampler, TexCoord).x;
}
float2 GetParallax(in float2 TexCoord, in float2 Direction, in float Amount, in float Bias)
{
    float Height = HeightMap.SampleLevel(Sampler, TexCoord, 0).x;
    return TexCoord + Direction * (Height * Amount + Bias);
}
float3 GetNormal(in float2 TexCoord, in float3 Normal, in float3 Tangent, in float3 Bitangent)
{
    float3 Result = NormalMap.Sample(Sampler, TexCoord).xyz * 2.0 - 1.0;
    return normalize(Result.x * Tangent + Result.y * Bitangent + Result.z * Normal);
}
float4 GetDiffuse(in float2 TexCoord)
{
    return DiffuseMap.Sample(Sampler, TexCoord);
}
float4 GetDiffuseLevel(in float2 TexCoord, in float Level)
{
    return DiffuseMap.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetSample(in Texture2D Texture, in float2 TexCoord)
{
    return Texture.Sample(Sampler, TexCoord);
}
float4 GetSampleLevel(in Texture2D Texture, in float2 TexCoord, in float Level)
{
    return Texture.SampleLevel(Sampler, TexCoord, Level);
}
Material GetMaterial(in float Material_Id)
{
    return Materials[Material_Id];
}