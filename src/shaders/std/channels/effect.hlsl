#include "std/buffers/viewer.hlsl"
#include "std/objects/material.hlsl"
#include "std/objects/fragment.hlsl"

StructuredBuffer<Material> Materials : register(t0);
Texture2D DiffuseBuffer : register(t1);
Texture2D NormalBuffer : register(t2);
Texture2D DepthBuffer : register(t3);
Texture2D SurfaceBuffer : register(t4);
SamplerState Sampler : register(s0);

bool IsInPixelGrid(float2 TexCoord)
{
	return TexCoord.x >= 0.0 && TexCoord.x <= 1.0 && TexCoord.y >= 0.0 && TexCoord.y <= 1.0;
}
float Linearize(float Depth)
{
	return vb_Near * vb_Far / (vb_Far + Depth * (vb_Near - vb_Far));
}
float GetDepth(float2 TexCoord)
{
	return DepthBuffer.SampleLevel(Sampler, TexCoord, 0).x;
}
float3 GetNormal(float2 TexCoord)
{
	return NormalBuffer.SampleLevel(Sampler, TexCoord, 0).xyz;
}
float4 GetDiffuse(float2 TexCoord, float Level)
{
	return DiffuseBuffer.SampleLevel(Sampler, TexCoord, Level);
}
float4 GetDiffuseSample(float2 TexCoord)
{
	return DiffuseBuffer.Sample(Sampler, TexCoord);
}
Fragment GetFragment(float2 TexCoord)
{
	float4 C0 = DiffuseBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 C1 = NormalBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 C2 = DepthBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 C3 = SurfaceBuffer.SampleLevel(Sampler, TexCoord, 0);
	float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, C2.x, 1.0), vb_InvViewProj);

	Fragment Result;
	Result.Position = Position.xyz / Position.w;
	Result.Diffuse = C0.xyz;
	Result.Alpha = C0.w;
	Result.Normal = C1.xyz;
	Result.Material = C1.w;
	Result.Depth = C2.x;
	Result.Roughness = C3.x;
	Result.Metallic = C3.y;
	Result.Occlusion = C3.z;
	Result.Emission = C3.w;

	return Result;
}