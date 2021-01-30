#include "std/layouts/skin"
#include "std/channels/depth"
#include "std/buffers/object"
#include "std/buffers/animation"

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

		Result.Position = Result.UV = mul(mul(float4(V.Position, 1.0), Offset), WorldViewProjection);
	}
	else
		Result.Position = Result.UV = mul(float4(V.Position, 1.0), WorldViewProjection);

	return Result;
}