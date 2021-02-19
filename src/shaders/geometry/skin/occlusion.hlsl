#include "std/layouts/skin"
#include "std/buffers/object"
#include "std/buffers/animation"

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	[branch] if (HasAnimation > 0)
	{
		matrix Offset =
			mul(Offsets[(int)V.Index.x], V.Bias.x) +
			mul(Offsets[(int)V.Index.y], V.Bias.y) +
			mul(Offsets[(int)V.Index.z], V.Bias.z) +
			mul(Offsets[(int)V.Index.w], V.Bias.w);

		Result.Position = mul(mul(float4(V.Position, 1.0), Offset), WorldViewProjection);
    }
	else
		Result.Position = mul(float4(V.Position, 1.0), WorldViewProjection);

	return Result;
}