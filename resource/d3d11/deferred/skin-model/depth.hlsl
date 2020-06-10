#include "renderer/vertex/skin"
#include "renderer/buffer/object"
#include "renderer/buffer/animation"
#include "geometry/forward"

VertexResult90 VS(VertexBase V)
{
	VertexResult90 Result = (VertexResult90)0;
	Result.TexCoord = V.TexCoord * TexCoord;

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

float4 PS(VertexResult90 V) : SV_TARGET0
{
    Material Mat = GetMaterial(MaterialId);

	float Alpha = Mat.Transparency.x;
	[branch] if (HasDiffuse > 0)
		Alpha *= GetDiffuse(V.TexCoord).w;

	[branch] if (Alpha < Mat.Transparency.y)
		discard;

	return float4(V.UV.z / V.UV.w, Alpha, 1.0f, 1.0f);
};