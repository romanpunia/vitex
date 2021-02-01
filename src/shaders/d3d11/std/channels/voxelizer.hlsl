#include "std/core/material"
#include "std/buffers/viewer"
#include "std/buffers/voxelizer"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<unorm float4> LightBuffer : register(u1);
RWTexture3D<unorm float4> DiffuseBuffer : register(u2);
RWTexture3D<unorm float4> NormalBuffer : register(u3);
RWTexture3D<unorm float4> SurfaceBuffer : register(u4);

float3 GetVoxelToWorld(uint3 Position)
{
    return (float3)Position * (VxScale / VxSize) + VxCenter;
}
Fragment GetFragmentWithDiffuse(float4 C0, uint3 Voxel)
{
    float4 C1 = NormalBuffer[Voxel];
    float4 C3 = SurfaceBuffer[Voxel];

    Fragment Result;
    Result.Position = GetVoxelToWorld(Voxel);
    Result.Diffuse = C0.xyz;
    Result.Alpha = C0.w;
    Result.Normal = C1.xyz;
    Result.Material = C1.w;
    Result.Depth = 0.0;
    Result.Roughness = C3.x;
    Result.Metallic = C3.y;
    Result.Occlusion = C3.z;
    Result.Emission = C3.w;

    return Result;
}
Fragment GetFragment(uint3 Voxel)
{
    float4 C0 = DiffuseBuffer[Voxel];
    return GetFragmentWithDiffuse(C0, Voxel);
}
Material GetMaterial(float Material_Id)
{
    return Materials[floor(Material_Id)];
}