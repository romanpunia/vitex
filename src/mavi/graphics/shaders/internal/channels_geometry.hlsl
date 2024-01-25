#include "internal/objects_material.hlsl"
#include "internal/buffers_viewer.hlsl"
#pragma warning(disable: 4000)

StructuredBuffer<Material> Materials : register(t0);
Texture2D DiffuseMap : register(t1);
Texture2D NormalMap : register(t2);
Texture2D MetallicMap : register(t3);
Texture2D RoughnessMap : register(t4);
Texture2D HeightMap : register(t5);
Texture2D OcclusionMap : register(t6);
Texture2D EmissionMap : register(t7);
SamplerState Sampler : register(s1);

float2 GetParallax(float2 TexCoord, float3 Direction, float Amount, float Bias)
{
	float Steps = lerp(32.0, 8.0, pow(1.0 - abs(dot(float3(0.0, 0.0, 1.0), Direction)), 4));
	float Step = 1.0 / Steps;
	float Depth = 0.0;
	float2 Delta = Direction.xy * Amount / Steps;
	float2 Result = TexCoord;
	float Sample = HeightMap.SampleLevel(Sampler, Result, 0).x + Bias;

	[loop] for (float i = 0; i < Steps; i++)
	{
		[branch] if (Depth >= Sample)
			break;

		Result -= Delta;
		Sample = HeightMap.SampleLevel(Sampler, Result, 0).x + Bias;  
		Depth += Step;
	}

	float2 Origin = Result + Delta;
	float Depth1 = Sample - Depth;
	float Depth2 = HeightMap.SampleLevel(Sampler, Origin, 0).x + Bias - Depth + Step;
	float Weight = Depth1 / (Depth1 - Depth2);
	
	return Origin * Weight + Result * (1.0 - Weight);
}
float3 GetDirection(float3 Tangent, float3 Bitangent, float3 Normal, float4 Position, float2 Scale)
{
	float3x3 TangentSpace;
	TangentSpace[0] = Tangent;
	TangentSpace[1] = Bitangent;
	TangentSpace[2] = Normal;
	TangentSpace = transpose(TangentSpace);

	return mul(normalize(vb_Position - Position.xyz), TangentSpace) / float3(Scale, 1.0);
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