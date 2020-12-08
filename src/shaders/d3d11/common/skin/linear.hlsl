#include "sdk/layouts/skin"
#include "sdk/channels/gflux"
#include "sdk/buffers/object"
#include "sdk/buffers/animation"

VOutputLinear VS(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.TexCoord = V.TexCoord * TexCoord;
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));

	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

		Result.Position = Result.UV = mul(mul(V.Position, Offset), WorldViewProjection);
	}
	else
		Result.Position = Result.UV = mul(V.Position, WorldViewProjection);

	return Result;
}