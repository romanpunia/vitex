#include "std/buffers/viewer.hlsl"
#include "std/buffers/voxelizer.hlsl"
#include "std/core/material.hlsl"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
RWTexture3D<unorm float4> DiffuseBuffer : register(u1);
RWTexture3D<float4> NormalBuffer : register(u2);
RWTexture3D<unorm float4> SurfaceBuffer : register(u3);
Texture2D DiffuseMap : register(t4);
Texture2D NormalMap : register(t5);
Texture2D MetallicMap : register(t6);
Texture2D RoughnessMap : register(t7);
Texture2D OcclusionMap : register(t8);
Texture2D EmissionMap : register(t9);
SamplerState Sampler : register(s4);

uint GetVoxelDominant(float3 N1, float3 N2, float3 N3)
{
	const float3 Pivot = abs(cross(N2.xyz - N1.xyz, N3.xyz - N1.xyz));
	uint Dominant = 2;

	[branch] if (Pivot.z > Pivot.x && Pivot.z > Pivot.y)
		Dominant = 0;
	else if (Pivot.x > Pivot.y && Pivot.x > Pivot.z)
		Dominant = 1;
	
	return Dominant;
}
float4 GetVoxelPosition(float4 Position, uint Dominant)
{
	[branch] if (Dominant == 0)
		return float4(Position.xy, 0, 1);
	else if (Dominant == 1)
		return float4(Position.yz, 0, 1);
	
	return float4(Position.xz, 1.0, 1.0);
}
float4 GetVoxel(float4 Position)
{
	return float4(clamp(Position.xyz - vxb_Center, -vxb_Scale, vxb_Scale) / vxb_Scale, 1.0);
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
void Compose(float2 TexCoord, float4 Diffuse, float3 Normal, float3 Position, float MaterialId)
{
	[branch] if (Position.x < -1.0 || Position.x > 1.0 || Position.y < -1.0 || Position.y > 1.0 || Position.z < -1.0 || Position.z > 1.0)
		return;
	
	uint3 Voxel = (uint3)floor((float3(0.5, -0.5, 0.5) * Position + 0.5) * vxb_Size);
	DiffuseBuffer[Voxel] = Diffuse;
	NormalBuffer[Voxel] = float4(Normal, MaterialId);
	SurfaceBuffer[Voxel] = float4(
		RoughnessMap.Sample(Sampler, TexCoord).x,
		MetallicMap.Sample(Sampler, TexCoord).x,
		OcclusionMap.Sample(Sampler, TexCoord).x,
		EmissionMap.Sample(Sampler, TexCoord).x);
}