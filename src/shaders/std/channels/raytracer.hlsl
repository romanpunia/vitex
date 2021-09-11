#include "std/buffers/viewer.hlsl"
#include "std/buffers/voxelizer.hlsl"
#include "std/core/material.hlsl"
#include "std/core/lighting.hlsl"
#include "std/core/position.hlsl"

StructuredBuffer<Material> Materials : register(t0);
Texture2D DiffuseBuffer : register(t1);
Texture2D NormalBuffer : register(t2);
Texture2D DepthBuffer : register(t3);
Texture2D SurfaceBuffer : register(t4);
Texture3D<unorm float4> LightBuffer : register(t5);
SamplerState Sampler : register(s1);

bool IsInVoxelGrid(float3 Voxel)
{
	return (Voxel.x > 0.0 && Voxel.x < 1.0 && Voxel.y > 0.0 && Voxel.y < 1.0 && Voxel.z > 0.0 && Voxel.z < 1.0);
}
float GetAvg(float3 Value)
{
	return (Value.x + Value.y + Value.z) / 3.0;
}
float3 GetVoxel(float3 Position)
{
	float3 Voxel = clamp(Position - vxb_Center, -vxb_Scale, vxb_Scale) / vxb_Scale;
	return (float3(0.5, -0.5, 0.5) * Voxel + 0.5);
}
float3 GetFlatVoxel(float3 Voxel)
{
	return floor(Voxel * vxb_Size) / vxb_Size;
}
float4 GetLight(float3 Voxel, float Level)
{
	return LightBuffer.SampleLevel(Sampler, Voxel, Level);
}
float4 GetDiffuse(float2 TexCoord)
{
	return DiffuseBuffer.SampleLevel(Sampler, TexCoord, 0);
}
Fragment GetFragment(float2 TexCoord)
{
	float4 C0 = DiffuseBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 C1 = NormalBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 C2 = DepthBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 C3 = SurfaceBuffer.SampleLevel(Sampler, TexCoord, 0);
	float3 Position = GetPosition(TexCoord, C2.x);
	bool Usable = IsInVoxelGrid(GetVoxel(Position));

	Fragment Result;
	Result.Position = Position;
	Result.Diffuse = C0.xyz;
	Result.Alpha = C0.w;
	Result.Normal = C1.xyz;
	Result.Material = C1.w;
	Result.Depth = (Usable ? C2.x : 2.0);
	Result.Roughness = C3.x;
	Result.Metallic = C3.y;
	Result.Occlusion = C3.z;
	Result.Emission = C3.w;

	return Result;
}