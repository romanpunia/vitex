#include "std/buffers/viewer"
#include "std/buffers/voxelizer"
#include "std/objects/lumina"
#include "std/core/material"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<unorm float4> LightBuffer : register(u1);
RWTexture3D<unorm float4> DiffuseBuffer : register(u2);
RWTexture3D<float4> NormalBuffer : register(u3);
RWTexture3D<unorm float4> SurfaceBuffer : register(u4);
Texture2D DiffuseMap : register(t5);
Texture2D NormalMap : register(t6);
Texture2D MetallicMap : register(t7);
Texture2D RoughnessMap : register(t8);
Texture2D OcclusionMap : register(t9);
Texture2D EmissionMap : register(t10);
SamplerState Sampler : register(s0);

void ConvervativeRasterize(inout float4 P1, inout float4 P2, inout float4 P3)
{
    float2 Side0 = normalize(P2.xy - P1.xy);
    float2 Side1 = normalize(P3.xy - P2.xy);
    float2 Side2 = normalize(P1.xy - P3.xy);
    P1.xy += normalize(Side2 - Side0) / VxSize.xy;
    P2.xy += normalize(Side0 - Side1) / VxSize.xy;
    P3.xy += normalize(Side1 - Side2) / VxSize.xy;
}
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
    
    return float4(Position.xy, 1.0, 1.0);
}
float4 GetVoxel(float4 Position)
{
    return float4(clamp(Position.xyz - VxCenter, -VxScale, VxScale) / VxScale, 1.0);
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
    [branch] if (Position.x < -1.0 || Position.x > 1.0 || Position.y < -1.0 || Position.y > 1.0 || Position.z < -1.0 || Position.z > 1.0)
        return (Lumina)0;
    
    Material Mat = Materials[MaterialId];
    float Emission = EmissionMap.Sample(Sampler, TexCoord).x;
    uint3 Voxel = (uint3)floor((float3(0.5, -0.5, 0.5) * Position + 0.5) * VxSize);
    float3 Light = (Mat.Emission.xyz + Emission) * Mat.Emission.w;
    
    LightBuffer[Voxel] = float4(Light, min(1.0, Mat.Emission.w));
    DiffuseBuffer[Voxel] = Diffuse;
    NormalBuffer[Voxel] = float4(Normal, MaterialId);
    SurfaceBuffer[Voxel] = float4(
        RoughnessMap.Sample(Sampler, TexCoord).x,
        MetallicMap.Sample(Sampler, TexCoord).x,
        OcclusionMap.Sample(Sampler, TexCoord).x, 1.0);

    return (Lumina)0;
}
Material GetMaterial(float Material_Id)
{
    return Materials[floor(Material_Id)];
}