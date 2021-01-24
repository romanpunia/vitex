#include "sdk/buffers/viewer"
#include "sdk/objects/material"
#include "sdk/objects/fragment"
#include "sdk/buffers/voxelizer"

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<unorm float4> DiffuseLumina : register(u1);
RWTexture3D<float4> NormalLumina : register(u2);
RWTexture3D<float4> SurfaceLumina : register(u3);
Texture2D DepthBuffer : register(t4);
SamplerState Sampler : register(s0);

float GetDepth(float2 TexCoord)
{
    return DepthBuffer.SampleLevel(Sampler, TexCoord, 0).x;
}
uint3 GetVoxel(float3 Position, out bool Bounds)
{
    float3 Voxel = clamp(Position - GridCenter.xyz, -GridScale.xyz, GridScale.xyz) / GridScale.xyz;
    Bounds = (Voxel.x > -1.0 && Voxel.x < 1.0 && Voxel.y > -1.0 && Voxel.y < 1.0 && Voxel.z > -1.0 && Voxel.z < 1.0);
    return (uint3)floor((float3(0.5, -0.5, 0.5) * Voxel + 0.5) * GridSize.xyz);
}
Fragment GetFragment(float2 TexCoord)
{
    float4 C2 = DepthBuffer.SampleLevel(Sampler, TexCoord, 0);
    float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, C2.x, 1.0), InvViewProjection);
    Position /= Position.w;

    bool Bounds;
    uint3 Voxel = GetVoxel(Position.xyz, Bounds);
    float4 C0 = (Bounds ? DiffuseLumina[Voxel] : float4(0.0, 0.0, 0.0, 0.0));
    float4 C1 = (Bounds ? NormalLumina[Voxel] : float4(0.0, 0.0, 0.0, 0.0));
    float4 C3 = (Bounds ? SurfaceLumina[Voxel] : float4(0.0, 0.0, 0.0, 0.0));

    Fragment Result;
    Result.Position = Position.xyz;
    Result.Diffuse = C0.xyz;
    Result.Alpha = C0.w;
    Result.Normal = C1.xyz;
    Result.Material = C1.w;
    Result.Depth = (Bounds ? C2.x : 1.0);
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