#include "sdk/layouts/shape"
#include "sdk/channels/raytracer"
#include "sdk/core/raytracing"
#include "sdk/core/position"
#include "sdk/core/lighting"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
    [branch] if (Frag.Depth >= 1.0)
        return float4(0, 0, 0, 0);
    
    float Steps = 128.0;
    float3 Start = Frag.Position + Frag.Normal * 0.5;
    float3 Eye = normalize(Frag.Position - ViewPosition);
	float3 Direction = reflect(Eye, Frag.Normal);
    float3 Step = GridScale.xyz / (2.0 * Steps);
    float3 Distance = Step * 0.1;
	float3 Result = 0.0;
    float Alpha = 0.0;

    [loop] for (float i = 0; i < Steps; i++)
    {
		float3 Voxel = GetVoxel(Start + Direction * Distance);
        [branch] if (!IsInVoxelGrid(Voxel))
            return float4(0, 0, 0, 0);
        
        float4 Diffuse = GetDiffuse(Voxel, 0);
        [branch] if (Diffuse.w > 0.75)
        {
            Diffuse *= (1.0 - Alpha);
            Result += Diffuse.xyz;
            Alpha += Diffuse.w;
        }

        Distance += Step;
    }

	Material Mat = GetMaterial(Frag.Material);
    float T = GetRoughnessMip(Frag, Mat, 7.0);
    float R = GetRoughness(Frag, Mat);
	float3 M = GetMetallic(Frag, Mat);
    float3 C = GetSpecularBRDF(Frag.Normal, -Eye, normalize(Direction), Result, M, R);

	return float4(C, Alpha);
};