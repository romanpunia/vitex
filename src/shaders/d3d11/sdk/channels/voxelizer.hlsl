#include "sdk/objects/material"
#include "sdk/objects/lumina"
#include "sdk/buffers/viewer"
#include "sdk/buffers/voxelizer"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<float4> DiffuseLumina : register(u1);
RWTexture3D<float4> NormalLumina : register(u2);
RWTexture3D<float4> SurfaceLumina : register(u3);
Texture2D DiffuseMap : register(t4);
Texture2D NormalMap : register(t5);
Texture2D MetallicMap : register(t6);
Texture2D RoughnessMap : register(t7);
Texture2D OcclusionMap : register(t8);
Texture2D EmissionMap : register(t9);
SamplerState Sampler : register(s0);

uint GetDominant(float3 N1, float3 N2, float3 N3)
{
    float3 Face = abs(N1 + N2 + N3);
	uint Dominant = Face[1] > Face[0] ? 1 : 0;
    return Face[2] > Face[Dominant] ? 2 : Dominant;
}
float4 GetVoxelSpace(float4 Position, uint Dominant)
{
    [branch] if (Dominant == 0)
        return float4(Position.zy, 1.0, 1.0);
    else if (Dominant == 1)
        return float4(Position.xz, 1.0, 1.0);
    else
        return float4(Position.xy, 1.0, 1.0);
}
float4 GetVoxel(float4 Position)
{
    return (Position - float4(GridCenter.xyz, 1.0)) / float4(GridScale.xyz, 1.0);
}
float3 GetNormal(float2 TexCoord, float3 Normal, float3 Tangent, float3 Bitangent)
{
    float3 Result = NormalMap.Sample(Sampler, TexCoord).xyz * 2.0 - 1.0;
    return normalize(Result.x * Tangent + Result.y * Bitangent + Result.z * Normal);
}
float4 GetDiffuse(float2 TexCoord)
{
    return DiffuseMap.Sample(Sampler, TexCoord);
}
Lumina Compose(float2 TexCoord, float4 Diffuse, float3 Normal, float3 Position, float MaterialId)
{
    uint3 Voxel = (uint3)floor((float3(0.5, -0.5, 0.5) * Position + 0.5) * GridSize.xyz);
    [branch] if (Voxel.x > (uint)GridSize.x || Voxel.y > (uint)GridSize.y || Voxel.z > (uint)GridSize.z)
        return (Lumina)0;
    
    DiffuseLumina[Voxel] = Diffuse;
    NormalLumina[Voxel] = float4(Normal, MaterialId);
    SurfaceLumina[Voxel] = float4(
        RoughnessMap.Sample(Sampler, TexCoord).x,
        MetallicMap.Sample(Sampler, TexCoord).x,
        OcclusionMap.Sample(Sampler, TexCoord).x,
        EmissionMap.Sample(Sampler, TexCoord).x);

    return (Lumina)0;
}