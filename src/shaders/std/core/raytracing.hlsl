#include "std/channels/raytracer.hlsl"

float3 Rayproject(float3 Normal)
{
	const float3 Base = float3(0.99146, 0.11664, 0.05832);
	const float3 Edge = float3(0.0, 1.0, 0.0);
	
	return abs(dot(Normal, Base)) > 0.99999 ? cross(Normal, Edge) : cross(Normal, Base);
}
float3 RaymarchSpecular(float3 Position, float3 Normal, float3 Direction, float MaxDistance, float Reflectivity)
{
	const float3 Intensity = 1.0 + Reflectivity;
	const float3 Offset = vxb_Distance / vxb_Scale;
	const float3 Step = 1.0 / vxb_Scale;
	float3 Distance = Offset;
	float4 Result = 0.0;
	float Count = 0.0;

	Position += Offset * Normal;
	while (Distance.x < MaxDistance && Result.w < 1.0)
	{
		float3 Voxel = GetVoxel(Position + Direction * Distance);
		[branch] if (!IsInVoxelGrid(Voxel) || Count++ >= vxb_MaxSteps)
			break;

		float Radius = 0.1 * Reflectivity * log2(1.0 + GetAvg(Distance * vxb_Scale));
		float4 Diffuse = GetLight(Voxel, min(Radius * vxb_Mips, vxb_Mips));
		float Volume = 1.0 - Result.w;
		Result.xyz += Intensity * Diffuse.xyz * Diffuse.w * Volume;
		Result.w += Diffuse.a * Volume;
		Distance += Step * (1.0f + 0.125f * Radius);
	}

	return Result.xyz;
}
float3 RaymarchRadiance(float3 Origin, float3 Direction, out float Occlusion)
{
	const float3 Step = vxb_Distance / vxb_Scale;
	const float Offset = 1.154;
	const float Depth = 0.03;
	float3 Distance = Step * 0.5;
	float4 Result = 0.0;
	float Count = 0.0;

	while (Distance.x < vxb_Scale.x && Result.w < 1.0)
	{
		float3 Voxel = GetVoxel(Origin + Direction * Distance);
		[branch] if (!IsInVoxelGrid(Voxel) || Count++ >= vxb_MaxSteps)
			break;
		
		float3 Radius = max(Offset * Distance, Step);
		float Roughness = log2(GetAvg(Radius * vxb_Scale));
		float4 Diffuse = GetLight(Voxel, min(Roughness, vxb_Mips));
		Result += Diffuse * (1.0 - Result.w);
		Distance += Radius * vxb_Step / max(1.0, Roughness);
		Occlusion += Diffuse.w / (1.0 + Depth * GetAvg(Radius));
	}

	return Result.xyz * Result.w;
}
float3 GetSpecular(float3 Position, float3 Normal, float3 Eye, float MaxDistance, float3 Metallic, float Roughness)
{
	float3 Direction = reflect(Eye, Normal);
	return RaymarchSpecular(Position, Normal, Direction, MaxDistance, Roughness) * Metallic;
}
float3 GetRefraction(float3 Position, float3 Normal, float3 Eye, float MaxDistance, float3 Metallic, float Roughness, float Refraction, float Transparency)
{
	float3 Direction = refract(Eye, Normal, 1.0 / Refraction);
	float3 Diffuse = lerp(Metallic, 0.5 * (Metallic + 1.0), Transparency);
	return RaymarchSpecular(Position, Normal, Direction, MaxDistance, Roughness) * Diffuse;
}
float3 GetRadiance(float3 Position, float3 Normal, float3 Metallic, out float Shadow)
{
	const float3 Origin = Position + Normal * vxb_Margin / vxb_Scale;
	const float3 Ortho1 = normalize(Rayproject(Normal));
	const float3 Ortho2 = normalize(cross(Ortho1, Normal));
	const float3 Corner1 = 0.5f * (Ortho1 + Ortho2);
	const float3 Corner2 = 0.5f * (Ortho1 - Ortho2);
	float Occlusion = 0.0;
	float3 Result = 0.0;

	Result += RaymarchRadiance(Origin + vxb_Offset * Normal, Normal, Occlusion);
	Result += RaymarchRadiance(Origin + vxb_Offset * Ortho1, lerp(Normal, Ortho1, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin - vxb_Offset * Ortho1, lerp(Normal, -Ortho1, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin + vxb_Offset * Ortho2, lerp(Normal, Ortho2, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin - vxb_Offset * Ortho2, lerp(Normal, -Ortho2, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin + vxb_Offset * Corner1, lerp(Normal, Corner1, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin - vxb_Offset * Corner1, lerp(Normal, -Corner1, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin + vxb_Offset * Corner2, lerp(Normal, Corner2, vxb_Angle), Occlusion);
	Result += RaymarchRadiance(Origin - vxb_Offset * Corner2, lerp(Normal, -Corner2, vxb_Angle), Occlusion);
	Shadow = saturate(1.0 - Occlusion * vxb_Occlusion);

	return Result;
}