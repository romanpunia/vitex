#include "sdk/channels/raytracer"

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

float4 Conetrace(float3 Position, float3 Normal, float3 Direction, float Ratio, out float Occlusion)
{
	float3 Step = 1.0 / VxScale;
    float3 Origin = Position + Normal * Step * 2.828426;
	float3 Distance = Step * 0.5;
	float4 Result = 0.0;
    float Count = 0.0;

	while (Distance.x < VxScale.x && Result.w < 1.0)
	{
		float3 Position = GetVoxel(Origin + Direction * Distance);
        [branch] if (!IsInVoxelGrid(Position) || Count++ >= VxMaxSteps)
            break;
        
		float3 Radius = max(2.0 * Distance * Ratio, Step);
		float Level = GetAvg(log2(Radius * VxScale));
        [branch] if (Level > VxMipLevels)
            break;

        float4 Diffuse = GetDiffuse(Position, Level);
		Result += Diffuse * (1.0 - Result.w);
		Distance += Radius * VxStep / max(1.0, Level);
        Occlusion += Diffuse.w / (1.0 + 0.03 * GetAvg(Radius));
	}

	return Result;
}
float4 GetReflectance(float3 Position, float3 Normal, float3 Metallic, float Roughness)
{
    float Occlusion = 0.0;
    float3 Eye = normalize(Position - ViewPosition);
	float3 Direction = reflect(Eye, Normal);
	float Ratio = GetAperture(Roughness);
    float4 Result = Conetrace(Position, Normal, Direction, Ratio, Occlusion);

    return float4(GetReflectanceBRDF(Normal, -Eye, Direction, Result.xyz, Metallic, Roughness), Result.w);
}
float4 GetRadiance(float3 Position, float3 Normal, float3 Metallic)
{
    float Occlusion = 0.0;
	float4 Result = 0.0;

	[unroll] for (uint i = 0; i < 6; i++)
	{
		float3 Direction = normalize(ConeDirections[i] + Normal);
		Direction *= dot(Direction, Normal) < 0.0 ? -1.0 : 1.0;

        float Ambient = 0.0;
        Result += ConeWeights[i] * Conetrace(Position, Normal, Direction, 0.577, Ambient);
        Occlusion += ConeWeights[i] * Ambient;
	}

    Occlusion = saturate(1.0 - Occlusion * VxShadows);
    Occlusion = lerp(1.0, min(1.0, Occlusion), VxOcclusion);
	Result = saturate(Result / 6.0);
    
    return float4(Result.xyz * Occlusion, Result.w);;
}