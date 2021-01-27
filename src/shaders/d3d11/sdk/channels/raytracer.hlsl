#include "sdk/buffers/viewer"
#include "sdk/buffers/voxelizer"
#include "sdk/core/material"
#include "sdk/core/lighting"

StructuredBuffer<Material> Materials : register(t0);
Texture3D<unorm float4> LightBuffer : register(t1);
Texture2D NormalBuffer : register(t2);
Texture2D DepthBuffer : register(t3);
Texture2D SurfaceBuffer : register(t4);
SamplerState Sampler : register(s0);

bool IsInVoxelGrid(float3 Voxel)
{
    return (Voxel.x > 0.0 && Voxel.x < 1.0 && Voxel.y > 0.0 && Voxel.y < 1.0 && Voxel.z > 0.0 && Voxel.z < 1.0);
}
float GetAvg(float3 Value)
{
    return (Value.x + Value.y + Value.z) / 3.0;
}
float GetFalloff(float3 Position)
{
    float Distance = (VxScale.x + VxScale.y + VxScale.z) / 3.0;
    return GetRangeAttenuation(VxCenter - Position, 0.8, 0.8, Distance);
}
float3 GetVoxel(float3 Position)
{
    float3 Voxel = clamp(Position - VxCenter, -VxScale, VxScale) / VxScale;
    return (float3(0.5, -0.5, 0.5) * Voxel + 0.5);
}
float3 GetFlatVoxel(float3 Voxel)
{
    return floor(Voxel * VxSize) / VxSize;
}
float4 GetDiffuse(float3 Voxel, float Level)
{
    return LightBuffer.SampleLevel(Sampler, Voxel, Level);
}
Fragment GetFragment(float2 TexCoord)
{
    float4 C2 = DepthBuffer.SampleLevel(Sampler, TexCoord, 0);
    float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, C2.x, 1.0), InvViewProjection);
    Position /= Position.w;

    float3 Voxel = GetFlatVoxel(GetVoxel(Position.xyz));
    float4 C0 = LightBuffer.SampleLevel(Sampler, Voxel, 0);
    float4 C1 = NormalBuffer.SampleLevel(Sampler, TexCoord, 0);
    float4 C3 = SurfaceBuffer.SampleLevel(Sampler, TexCoord, 0);
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
    return Materials[floor(Material_Id)];
}