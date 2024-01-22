#include "std/layouts/vertex-instance.hlsl"
#include "std/channels/depth.hlsl"
#include "std/buffers/object.hlsl"

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), V.OB_Transform);
	Result.TexCoord = V.TexCoord * V.OB_TexCoord.xy;
    Result.OB_Diffuse = V.OB_Material.x;
    Result.OB_MaterialId = V.OB_Material.w;

	return Result;
}

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (V.OB_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[V.OB_MaterialId].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return V.UV.z / V.UV.w;
};