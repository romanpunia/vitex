#include "sdk/channels/raytracer"

float4 Conetrace(float3 Normal, float3 Position, float3 Direction, float Ratio)
{
    Fragment Frag = (Fragment)0;
    Material Mat = (Material)0;
	float3 Step = 1.0 / GridScale.xyz;
    float3 Origin = Position + Normal * Step * 2.0 * 1.414213;
	float3 Distance = Step;
	float4 Result = 0.0;

	while (Distance.x < GridScale.x && Result.w < 1.0)
	{
		float3 Position = GetVoxel(Origin + Direction * Distance);
        [branch] if (!IsInVoxelGrid(Position))
            break;
        
		float3 Radius = max(2.0 * Distance * Ratio, Step);
		float Level = length(log2(Radius * GridScale.xyz)) / 3.0;
        [branch] if (Level > GridMipLevels)
            break;

        Frag = GetFragment3D(Position, Level);
        Mat = GetMaterial(Frag.Material);
        Frag.Diffuse += GetEmission(Frag, Mat);

		Result += float4(Frag.Diffuse * Frag.Alpha, Frag.Alpha) * (1.0 - Result.w);
		Distance += Radius * GridStep;
	}

	return Result;
}