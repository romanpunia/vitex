#include "std/layouts/skin.hlsl"
#include "std/channels/depth.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/animation.hlsl"

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	[branch] if (ab_Animated > 0)
	{
		matrix Offset =
			mul(ab_Offsets[(int)V.Index.x], V.Bias.x) +
			mul(ab_Offsets[(int)V.Index.y], V.Bias.y) +
			mul(ab_Offsets[(int)V.Index.z], V.Bias.z) +
			mul(ab_Offsets[(int)V.Index.w], V.Bias.w);

		Result.Position = Result.UV = mul(mul(float4(V.Position, 1.0), Offset), ob_Transform);
	}
	else
		Result.Position = Result.UV = mul(float4(V.Position, 1.0), ob_Transform);

	return Result;
}

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_MaterialId].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return V.UV.z / V.UV.w;
};