#include "std/channels/rvoxelizer"
#include "std/core/lighting"
#include "lighting/common/point/array"

static const float Alpha = 0.005;
static const float Bounce = 10.0;

[numthreads(8, 8, 8)]
void CS(uint3 Voxel : SV_DispatchThreadID)
{
	float4 Diffuse = GetDiffuse(Voxel);
    [branch] if (Diffuse.w < Alpha || (Diffuse.x < Alpha && Diffuse.y < Alpha && Diffuse.z < Alpha))
        return;

	Fragment Frag = GetFragmentWithDiffuse(Diffuse, Voxel);
	Material Mat = GetMaterial(Frag.Material);
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 E = GetSurface(Frag, Mat);
    float3 D = normalize(ViewPosition - Frag.Position);
    float4 Result = 0.0;

    [loop] for (float i = 0; i < VxLights; i++)
    {
        PointLight Light = Lights[i];
        float3 K = Light.Position - Frag.Position;
        float3 L = normalize(K);
        float3 R = GetReflectanceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        float3 S = GetSubsurface(Frag.Normal, D, L, Mat.Scatter) * E;
        float A = GetRangeAttenuation(K, Light.Attenuation.x, Light.Attenuation.y, Light.Range);
	
        Result += float4(Light.Lighting * (R + S), 1.0) * A;
    }

    Result.w /= max(1.0, VxLights);
    LightBuffer[Voxel] = Result;
};