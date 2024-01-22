#include "std/layouts/shape.hlsl"
#include "std/channels/raytracer.hlsl"
#include "std/core/raytracing.hlsl"
#include "std/core/position.hlsl"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), vxb_Transform);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	Fragment Frag = GetFragment(GetTexCoord(V.TexCoord));
	[branch] if (Frag.Depth >= 1.0)
		return float4(Frag.Diffuse, 1.0);

	Material Mat = Materials[Frag.Material];
	float R = GetRoughness(Frag, Mat), Shadow;
	float L = distance(Frag.Position, -1.0) * vxb_Length;
	float3 M = GetMetallic(Frag, Mat);
	float3 E = GetEmission(Frag, Mat);
	float3 D = normalize(Frag.Position - vb_Position);
	float3 Radiance = GetRadiance(Frag.Position, Frag.Normal, M, Shadow);
	float3 Reflectance = GetSpecular(Frag.Position, Frag.Normal, D, L, M, R);
	float3 Result = (Frag.Diffuse + vxb_Radiance * (E + Radiance) + vxb_Specular * Reflectance) * (1.0 - Mat.Transparency);

	[branch] if (Mat.Transparency > 0)
	{
		float3 Refraction = GetRefraction(Frag.Position, Frag.Normal, D, L, M, R, Mat.Refraction, Mat.Transparency);
		Result += vxb_Specular * Refraction;
	}

	return float4(Result * Shadow, Shadow);
};