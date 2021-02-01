#include "std/channels/voxelizer"
#include "std/core/lighting"
#include "lighting/common/point/array"

static const float Alpha = 0.01;

[numthreads(8, 8, 8)]
void CS(uint3 Voxel : SV_DispatchThreadID)
{
	float4 Diffuse = DiffuseBuffer[Voxel];
    [branch] if (Diffuse.w < Alpha || (Diffuse.x < Alpha && Diffuse.y < Alpha && Diffuse.z < Alpha))
        return;

	Fragment Frag = GetFragmentWithDiffuse(Diffuse, Voxel);
	Material Mat = GetMaterial(Frag.Material);
	float G = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 E = GetSurface(Frag, Mat);
    float3 D = normalize(ViewPosition - Frag.Position);
    float3 Result = 0.0;

    [loop] for (float i = 0; i < VxLights; i++)
    {
        PointLight Light = Lights[i];
        float3 K = Light.Position - Frag.Position * float3(1, 1, -1);
        float3 L = normalize(K);
        float3 R = GetReflectanceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        float3 S = GetSubsurface(Frag.Normal, D, L, Mat.Scatter) * E;
        float A = GetRangeAttenuation(K, Light.Attenuation.x, Light.Attenuation.y, Light.Range);
	
        Result += Light.Lighting * A;//(R + S) * A;
    }

    LightBuffer[Voxel] = float4(Result, 1.0);
};