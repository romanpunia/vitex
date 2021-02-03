#include "std/buffers/viewer"
#include "std/buffers/voxelizer"
#include "std/core/material"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<unorm float4> LightBuffer : register(u1);
Texture3D<unorm float4> DiffuseBuffer : register(t2);
Texture3D<float4> NormalBuffer : register(t3);
Texture3D<unorm float4> SurfaceBuffer : register(t4);

float3 GetVoxelToWorld(uint3 Position)
{
    float3 Result = (float3)Position / VxSize;
    Result = 2.0 * Result - 1.0;
    return Result * VxScale + VxCenter;
}
float4 GetDiffuse(uint3 Voxel)
{
    return DiffuseBuffer.Load(int4((int3)Voxel.xyz, 0));
}
float4 GetNormal(uint3 Voxel)
{
    return NormalBuffer.Load(int4((int3)Voxel.xyz, 0));
}
float4 GetSurface(uint3 Voxel)
{
    return SurfaceBuffer.Load(int4((int3)Voxel.xyz, 0));
}
Fragment GetFragmentWithDiffuse(float4 C0, uint3 Voxel)
{
    int4 Coord = int4((int3)Voxel.xyz, 0);
    float4 C1 = NormalBuffer.Load(Coord);
    float4 C3 = SurfaceBuffer.Load(Coord);

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
    float4 C0 = DiffuseBuffer.Load(int4((int3)Voxel.xyz, 0));
    return GetFragmentWithDiffuse(C0, Voxel);
}
Material GetMaterial(float Material_Id)
{
    return Materials[floor(Material_Id)];
}