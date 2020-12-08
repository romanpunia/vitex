#include "sdk/layouts/skin"
#include "sdk/buffers/object"
#include "sdk/buffers/animation"

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

		Result.Position = mul(mul(V.Position, Offset), WorldViewProjection);
    }
	else
		Result.Position = mul(V.Position, WorldViewProjection);

	return Result;
}