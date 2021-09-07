#include "std/channels/raytracer.hlsl"

static const float3 ConeDirections[] = 
{
	float3(0.0, 1.0, 0.0),
	float3(0.0, 0.5, 0.866025),
	float3(0.823639, 0.5, 0.267617),
	float3(0.509037, 0.5, -0.700629),
	float3(-0.509037, 0.5, -0.700629),
	float3(-0.823639, 0.5, 0.267617)
};
static const float ConeWeights[] =
{
	0.25,
	0.15,
	0.15,
	0.15,
	0.15,
	0.15
};

float4 Raymarch(float3 Position, float3 Normal, float3 Direction, float Ratio)
{
	float3 Step = vxb_Distance / vxb_Scale;
	float3 Origin = Position + Normal * Step * 2.828426;
	float3 Distance = Step * 0.5;
	float4 Result = 0.0;
	float Count = 0.0;

	while (Distance.x < vxb_Scale.x && Result.w < 1.0)
	{
		float3 Voxel = GetVoxel(Origin + Direction * Distance);
		[branch] if (!IsInVoxelGrid(Voxel) || Count++ >= vxb_MaxSteps)
			break;
		
		float3 Radius = max(2.0 * Distance * Ratio, Step);
		float Level = GetAvg(log2(Radius * vxb_Scale));
		[branch] if (Level > vxb_Mips)
			break;

		float4 Diffuse = GetLight(Voxel, Level);
		Result += Diffuse * (1.0 - Result.w);
		Distance += Radius * vxb_Step / max(1.0, Level);
	}

	return Result;
}
float4 RaymarchDensity(float3 Position, float3 Normal, float3 Direction, float Ratio, out float Occlusion)
{
	float3 Step = vxb_Distance / vxb_Scale;
	float3 Origin = Position + Normal * Step * 2.828426;
	float3 Distance = Step * 0.5;
	float4 Result = 0.0;
	float Count = 0.0;

	while (Distance.x < vxb_Scale.x && Result.w < 1.0)
	{
		float3 Voxel = GetVoxel(Origin + Direction * Distance);
		[branch] if (!IsInVoxelGrid(Voxel) || Count++ >= vxb_MaxSteps)
			break;
		
		float3 Radius = max(2.0 * Distance * Ratio, Step);
		float Level = GetAvg(log2(Radius * vxb_Scale));
		[branch] if (Level > vxb_Mips)
			break;

		float4 Diffuse = GetLight(Voxel, Level);
		Result += Diffuse * (1.0 - Result.w);
		Distance += Radius * vxb_Step / max(1.0, Level);
		Occlusion += Diffuse.w / (1.0 + 0.03 * GetAvg(Radius));
	}

	return Result;
}
float4 GetSpecular(float3 Position, float3 Normal, float3 Eye, float3 Metallic, float Roughness)
{
	float3 Direction = reflect(Eye, Normal);
	float Ratio = GetAperture(Roughness);
	float4 Result = Raymarch(Position, Normal, Direction, Ratio);

	return float4(GetReflectanceBRDF(Normal, -Eye, Direction, Result.xyz, Metallic, Roughness), Result.w);
}
float4 GetRadiance(float3 Position, float3 Normal, float3 Metallic, out float Shadow)
{
	float Occlusion = 0.0;
	float4 Result = 0.0;

	[unroll] for (uint i = 0; i < 6; i++)
	{
		float3 Direction = normalize(ConeDirections[i] + Normal);
		Direction *= dot(Direction, Normal) < 0.0 ? -1.0 : 1.0;

		float Ambient = 0.0;
		Result += ConeWeights[i] * RaymarchDensity(Position, Normal, Direction, 0.577, Ambient);
		Occlusion += ConeWeights[i] * Ambient;
	}

	Occlusion = saturate(1.0 - Occlusion * vxb_Shadows);
	Shadow = lerp(1.0, min(1.0, Occlusion), vxb_Occlusion);
	Result = saturate(Result / 6.0);
	
	return Result;
}