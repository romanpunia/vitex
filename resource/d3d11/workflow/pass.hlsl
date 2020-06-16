#include "renderer/material"
#include "renderer/fragment"
#include "renderer/buffer/viewer"
#include "renderer/input/sv"
#include "standard/pow"

float GetMaterialId(in float2 TexCoord)
{
    return NormalMap.SampleLevel(Sampler, TexCoord, 0).w;
}
float3 GetMetallicFactor(in Fragment Frag, in Material Mat)
{
    return Mat.Metallic.xyz + Frag.Metallic * Mat.Metallic.w;
}
float GetRoughnessLevel(in Fragment Frag, in Material Mat, in float MaxLevels)
{
    return MaxLevels * (Mat.Roughness.x + Frag.Roughness * Mat.Roughness.y);
}
float GetRoughnessFactor(in Fragment Frag, in Material Mat)
{
    return Pow4(Mat.Roughness.x + Frag.Roughness * Mat.Roughness.y);
}
float GetMetallic(in float2 TexCoord)
{
    return DepthMap.SampleLevel(Sampler, TexCoord, 0).y;
}
float GetRoughness(in float2 TexCoord)
{
    return DiffuseMap.SampleLevel(Sampler, TexCoord, 0).w;
}
float GetDepth(in float2 TexCoord)
{
    return DepthMap.SampleLevel(Sampler, TexCoord, 0).x;
}
float2 GetTexCoord(in float4 UV)
{
    return float2(0.5f + 0.5f * UV.x / UV.w, 0.5f - 0.5f * UV.y / UV.w);
}
float3 GetPosition(in float2 TexCoord, in float Depth)
{
    float4 Position = float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, Depth, 1.0);
    Position = mul(Position, InvViewProjection);

    return Position.xyz / Position.w;
}
float3 GetPositionUV(in float3 Position)
{
    float4 Coord = mul(float4(Position, 1.0), ViewProjection);
    Coord.xy = float2(0.5, 0.5) + float2(0.5f, -0.5) * Coord.xy / Coord.w;
    Coord.z /= Coord.w;
    
    return Coord.xyz;
}
float3 GetNormal(in float2 TexCoord)
{
    return NormalMap.SampleLevel(Sampler, TexCoord, 0).xyz;
}
float4 GetDiffuse(in float2 TexCoord)
{
    return DiffuseMap.SampleLevel(Sampler, TexCoord, 0);
}
float4 GetDiffuseLevel(in float2 TexCoord, in float Level)
{
    return DiffuseMap.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetPass(in float2 TexCoord)
{
    return PassMap.SampleLevel(Sampler, TexCoord, 0);
}
float4 GetPassLevel(in float2 TexCoord, in float Level)
{
    return PassMap.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetSample(in Texture2D Texture, in float2 TexCoord)
{
    return Texture.Sample(Sampler, TexCoord);
}
float4 GetSampleLevel(in Texture2D Texture, in float2 TexCoord, in float Level)
{
    return Texture.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetSample3(in TextureCube Texture, in float3 TexCoord)
{
    return Texture.Sample(Sampler, TexCoord);
}
float4 GetSample3Level(in TextureCube Texture, in float3 TexCoord, in float Level)
{
    return Texture.SampleLevel(Sampler, TexCoord, Level);
}
Fragment GetFragment(in float2 TexCoord)
{
    float2 Depth = DepthMap.SampleLevel(Sampler, TexCoord, 0).xy;
    float4 Diffuse = DiffuseMap.SampleLevel(Sampler, TexCoord, 0);
    float4 Normal = NormalMap.SampleLevel(Sampler, TexCoord, 0);

    Fragment Result;
    Result.Position = GetPosition(TexCoord, Depth.x);
    Result.Diffuse = Diffuse.xyz;
    Result.Normal = Normal.xyz;
    Result.Roughness = Diffuse.w;
    Result.Metallic = Depth.y;
    Result.Material = Normal.w;
    Result.Depth = Depth.x;

    return Result;
}
Material GetMaterial(in float Material_Id)
{
    return Materials[Material_Id];
}