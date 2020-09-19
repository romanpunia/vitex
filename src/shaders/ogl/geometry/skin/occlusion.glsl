#include "renderer/vertex/skin"
#include "renderer/buffer/object"
#include "renderer/buffer/animation"

VertexResult VS(VertexBase V)
{
	VertexResult Result = (VertexResult)0;
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