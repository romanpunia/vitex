#include "sdk/channels/raytracer"

float3 ConeTraceDiffuse(float3 From, float3 Direction)
{
	float3 Result = 0.0;
	float3 Distance = 0.1953125;
	float Spread = 0.325;

	while (length(Distance) / 3.0 < 1.414213)
    {
		float3 Voxel = GetVoxel(From + Distance * Direction);
        float3 L0 = Spread * Distance / GridScale.xyz;
		float L1 = log2(1.0 + (L0.x + L0.y + L0.z) / 3.0);
		float L2 = (L1 + 1.0) * (L1 + 1.0);

		Result += GetDiffuse(Voxel, 0).xyz;
		Distance += GridScale.xyz * L2 * 2.0;
	}

	return Result;
}