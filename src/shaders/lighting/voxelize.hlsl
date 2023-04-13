#include "std/channels/rvoxelizer.hlsl"
#include "std/core/lighting.hlsl"
#include "lighting/point/common/array.hlsl"
#include "lighting/spot/common/array.hlsl"
#include "lighting/line/common/array.hlsl"

static const float Alpha = 0.005;

float GetDensity(float Density)
{
	return Density / max(1.0, vxb_Lights.x + vxb_Lights.y + vxb_Lights.z);
}
float GetShadow(float3 Position, float3 Direction, float Length) 
{
	const float Steps = 48;
	float Step = Length / (Steps + 1.0);
	float Result = 0.0;

	for (float i = 0.0; i < Steps; i++)
	{
		float Distance = Step * (i + 2.0);
		float3 Voxel = Position + Direction * Distance;
		[branch] if (Result >= 1.0 || !IsInVoxelGrid(Voxel))
			break;

		float Density = ceil(GetDiffuse((uint3)Voxel).w);
		Result += (1.0 - Result) * Density / Distance;
	}

	return 1.0 - Result * Steps * vxb_Bleeding;
}

[numthreads(8, 8, 8)]
void cs_main(uint3 Voxel : SV_DispatchThreadID)
{
	float4 Diffuse = GetDiffuse(Voxel);
	[branch] if (Diffuse.w < Alpha || (Diffuse.x < Alpha && Diffuse.y < Alpha && Diffuse.z < Alpha))
		return;

	Fragment Frag = GetFragmentWithDiffuse(Diffuse, Voxel);
	Material Mat = Materials[Frag.Material];
    float Transparency = pow(1.0 - Mat.Transparency, 4);
    [branch] if (Transparency < Alpha)
        return;

	float G = GetRoughness(Frag, Mat), i;
	float3 M = GetMetallic(Frag, Mat);
	float3 D = normalize(vb_Position - Frag.Position);
	float4 Result = float4(GetEmission(Frag, Mat), 0.0);
	float3 Origin = (float3)Voxel;

	[loop] for (i = 0; i < vxb_Lights.x; i++)
	{
		PointLight Light = PointLights[i];
		float3 K = Light.Position - Frag.Position;
		float A = GetRangeAttenuation(K, Light.Attenuation.x, Light.Attenuation.y, Light.Range);
		[branch] if (A <= Alpha)
			continue;
		
		[branch] if (Light.Softness > 0.0 && vxb_Bleeding > 0.0)
		{
			float3 V = GetVoxel(Light.Position) - Origin;
			A *= GetShadow(Origin, normalize(V), length(V));
		}
		
		float3 L = normalize(K);
		float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
		Result += float4(Light.Lighting * R, 1.0) * A;
	}

	[loop] for (i = 0; i < vxb_Lights.y; i++)
	{
		SpotLight Light = SpotLights[i];
		float3 K = Light.Position - Frag.Position;
		float3 L = normalize(K);
		float A = GetConeAttenuation(K, L, Light.Attenuation.x, Light.Attenuation.y, Light.Range, Light.Direction, Light.Cutoff);
		[branch] if (A <= Alpha)
			continue;
		
		[branch] if (Light.Softness > 0.0 && vxb_Bleeding > 0.0)
		{
			float3 V = GetVoxel(Light.Position) - Origin;
			A *= GetShadow(Origin, normalize(V), length(V));
		}
		
		float3 R = GetCookTorranceBRDF(Frag.Normal, D, L, Frag.Diffuse, M, G);
		Result += float4(Light.Lighting * R, 1.0) * A;
	}

	[loop] for (i = 0; i < vxb_Lights.z; i++)
	{
		LineLight Light = LineLights[i]; float A = 1.0;
		[branch] if (Light.Softness > 0.0 && vxb_Bleeding > 0.0)
		{
			float3 V = Light.Position * float3(1.0, -1.0, 1.0);
			A = GetShadow(Origin, V, vxb_Size.x);
		}
		
		float3 R = GetCookTorranceBRDF(Frag.Normal, D, Light.Position, Frag.Diffuse, M, G);
		Result += float4(Light.Lighting * R, 1.0) * A;
	}

	Result.w = min(1.0, Mat.Emission.w + GetDensity(Result.w));
    LightBuffer[Voxel] += Result * Transparency;
};