#include "sdk/buffers/viewer"
#include "sdk/objects/material"
#include "sdk/objects/fragment"
#include "sdk/buffers/voxelizer"

StructuredBuffer<Material> Materials : register(t0);
Texture3D<unorm float4> DiffuseBuffer : register(t1);
Texture3D<float4> NormalBuffer : register(t2);
Texture3D<float4> SurfaceBuffer : register(t3);
Texture2D DepthBuffer : register(t4);
SamplerState Sampler : register(s0);

float IsInVoxelGrid(float3 Voxel)
{
    return (Voxel.x > 0.0 && Voxel.x < 1.0 && Voxel.y > 0.0 && Voxel.y < 1.0 && Voxel.z > 0.0 && Voxel.z < 1.0);
}
float GetGridFalloff(float3 Position)
{
    float Distance = (GridScale.x + GridScale.y + GridScale.z) / 3.0;
    float Falloff = length(GridCenter.xyz - Position);
    return saturate(Distance / (1.0 + 0.3 * Falloff + 0.5 * Falloff * Falloff));
}
float GetDepth(float2 TexCoord)
{
    return DepthBuffer.SampleLevel(Sampler, TexCoord, 0).x;
}
float3 GetVoxel(float3 Position)
{
    float3 Voxel = clamp(Position - GridCenter.xyz, -GridScale.xyz, GridScale.xyz) / GridScale.xyz;
    return (float3(0.5, -0.5, 0.5) * Voxel + 0.5);
}
float3 GetFlattenVoxel(float3 Voxel, float3 Smooth)
{
    return lerp(floor(Voxel * GridSize.xyz) / GridSize.xyz, Voxel, Smooth);
}
float4 GetDiffuse(float3 TexCoord, float Level)
{
    return DiffuseBuffer.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetNormal(float3 TexCoord, float Level)
{
    return NormalBuffer.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetSurface(float3 TexCoord, float Level)
{
    return SurfaceBuffer.SampleLevel(Sampler, TexCoord, Level);
}
Fragment GetFragment(float2 TexCoord)
{
    float4 C2 = DepthBuffer.SampleLevel(Sampler, TexCoord, 0);
    float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, C2.x, 1.0), InvViewProjection);
    Position /= Position.w;

    float3 Voxel = GetVoxel(Position.xyz);
    float4 C0 = DiffuseBuffer.SampleLevel(Sampler, Voxel, 0);
    float4 C1 = NormalBuffer.SampleLevel(Sampler, Voxel, 0);
    float4 C3 = SurfaceBuffer.SampleLevel(Sampler, Voxel, 0);
    bool Usable = IsInVoxelGrid(Voxel);

    Fragment Result;
    Result.Position = Position.xyz;
    Result.Diffuse = C0.xyz;
    Result.Alpha = C0.w;
    Result.Normal = C1.xyz;
    Result.Material = C1.w;
    Result.Depth = (Usable ? C2.x : 1.0);
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