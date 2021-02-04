#include "std/channels/rvoxelizer"
#include "std/core/lighting"
#include "lighting/point/common/array"
#include "lighting/spot/common/array"
#include "lighting/line/common/array"

static const float Alpha = 0.005;
static const float Bounce = 10.0;

float GetSamples()
{
    return max(1.0, VxLights.x + VxLights.y + VxLights.z);
}

[numthreads(8, 8, 8)]
void CS(uint3 Voxel : SV_DispatchThreadID)
{
	float4 Diffuse = GetDiffuse(Voxel);
    [branch] if (Diffuse.w < Alpha || (Diffuse.x < Alpha && Diffuse.y < Alpha && Diffuse.z < Alpha))
        return;

	Fragment Frag = GetFragmentWithDiffuse(Diffuse, Voxel);
	Material Mat = GetMaterial(Frag.Material);
	float G = GetRoughness(Frag, Mat), i;
	float3 M = GetMetallic(Frag, Mat);
    float3 D = normalize(ViewPosition - Frag.Position);
    float4 Result = float4(GetEmission(Frag, Mat), 0.0);

    [loop] for (i = 0; i < VxLights.x; i++)
    {
        PointLight Light = PointLights[i];
        float3 K = Light.Position - Frag.Position;
        float A = GetRangeAttenuation(K, Light.Attenuation.x, Light.Attenuation.y, Light.Range);

        [branch] if (A <= Alpha)
            continue;
        
        float3 L = normalize(K);
        float3 R = GetReflectanceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        Result += float4(Light.Lighting , 1.0) * A;
    }

    [loop] for (i = 0; i < VxLights.y; i++)
    {
        SpotLight Light = SpotLights[i];
        float3 K = Light.Position - Frag.Position;
        float3 L = normalize(K);
        float A = GetConeAttenuation(K, L, Light.Attenuation.x, Light.Attenuation.y, Light.Range, Light.Direction, Light.Cutoff);

        [branch] if (A <= Alpha)
            continue;

        float3 R = GetReflectanceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        Result += float4(Light.Lighting * R, 1.0) * A;
    }

    [loop] for (i = 0; i < VxLights.z; i++)
    {
        LineLight Light = LineLights[i];
        float3 L = normalize(Light.Position - Frag.Position);
        float3 R = GetReflectanceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        Result += float4(Light.Lighting * R, 1.0);
    }

    Result.w = min(1.0, Mat.Emission.w + Result.w / GetSamples());
    LightBuffer[Voxel] = Result;
};