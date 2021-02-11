#include "std/channels/rvoxelizer"
#include "std/core/lighting"
#include "lighting/point/common/array"
#include "lighting/spot/common/array"
#include "lighting/line/common/array"

static const float Alpha = 0.005;

float GetDensity(float Density)
{
    return Density / max(1.0, VxLights.x + VxLights.y + VxLights.z);
}
float GetShadow(float3 Position, float3 Direction, float Length) 
{
    float Step = 1.0 / VxSize.x;
    float Distance = Step * 2.0;
    float Result = 0.0;

    while (Result <= 1.0 && Distance <= Length) 
    {
        float3 Voxel = Direction * Distance + Position;
        [branch] if (!IsInVoxelGrid(Voxel)) 
            break; 
 
        float A = ceil(GetDiffuse((uint3)Voxel).w) * VxBleeding;
        Result += (1.0 - Result) * A / Distance;
        Distance += Step;
    }

    return 1.0 - Result;
}
float3 GetDirection(float3 Origin, float3 Dest)
{
    return GetVoxel(float3(Dest.x, -Dest.y, Dest.z)) - Origin;
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
    float3 Origin = (float3)Voxel;

    [loop] for (i = 0; i < VxLights.x; i++)
    {
        PointLight Light = PointLights[i];
        float3 K = Light.Position - Frag.Position;
        float A = GetRangeAttenuation(K, Light.Attenuation.x, Light.Attenuation.y, Light.Range);
        [branch] if (A <= Alpha)
            continue;
        
        [branch] if (Light.Softness > 0.0)
        {
            float3 V = GetDirection(Origin, Light.Position);
            A *= GetShadow(Origin, normalize(V), length(V));
            [branch] if (A <= Alpha)
                continue;
        }
        
        float3 L = normalize(K);
        float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        Result += float4(Light.Lighting * R, 1.0) * A;
    }

    [loop] for (i = 0; i < VxLights.y; i++)
    {
        SpotLight Light = SpotLights[i];
        float3 K = Light.Position - Frag.Position;
        float3 L = normalize(K);
        float A = GetConeAttenuation(K, L, Light.Attenuation.x, Light.Attenuation.y, Light.Range, Light.Direction, Light.Cutoff);
        [branch] if (A <= Alpha)
            continue;
        
        [branch] if (Light.Softness > 0.0)
        {
            float3 V = GetDirection(Origin, Light.Position);
            A *= GetShadow(Origin, normalize(V), length(V));
            [branch] if (A <= Alpha)
                continue;
        }
        
        float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
        Result += float4(Light.Lighting * R, 1.0) * A;
    }

    [loop] for (i = 0; i < VxLights.z; i++)
    {
        LineLight Light = LineLights[i];
        float A = 1.0;

        [branch] if (Light.Softness > 0.0)
        {
            A = GetShadow(Origin, float3(Light.Position.x, -Light.Position.y, Light.Position.z), VxSize.x);
            [branch] if (A <= Alpha)
                continue;
        }
        
        float3 R = GetCookTorranceBRDF(Frag.Normal, D, Light.Position, Frag.Diffuse, M, G);
        Result += float4(Light.Lighting * R, 1.0) * A;
    }

    Result.w = min(1.0, Mat.Emission.w + GetDensity(Result.w));
    LightBuffer[Voxel] = Result;
};