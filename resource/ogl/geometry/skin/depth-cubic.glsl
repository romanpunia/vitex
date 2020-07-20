#include "renderer/vertex/skin"
#include "renderer/buffer/object"
#include "renderer/buffer/cubic"
#include "renderer/buffer/animation"
#include "workflow/rasterizer"

[maxvertexcount(18)]
void GS(triangle VertexResult90 V[3], inout TriangleStream<VertexResult360> Stream)
{
	VertexResult360 Result = (VertexResult360)0;
	for (Result.RenderTarget = 0; Result.RenderTarget < 6; Result.RenderTarget++)
	{
		Result.Position = Result.UV = mul(V[0].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[0].UV;
		Result.TexCoord = V[0].TexCoord;
		Stream.Append(Result);

		Result.Position = Result.UV = mul(V[1].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[1].UV;
		Result.TexCoord = V[1].TexCoord;
		Stream.Append(Result);

		Result.Position = mul(V[2].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[2].UV;
		Result.TexCoord = V[2].TexCoord;
		Stream.Append(Result);

		Stream.RestartStrip();
	}
}

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

		Result.Position = Result.UV = mul(mul(V.Position, Offset), World);
	}
	else
		Result.Position = Result.UV = mul(V.Position, World);

	return Result;
}

float4 PS(VertexResult90 V) : SV_TARGET0
{
    Material Mat = GetMaterial(MaterialId);
	float Alpha = 1.0;

	[branch] if (HasDiffuse > 0)
		Alpha *= GetDiffuse(V.TexCoord).w;

	return float4(length(V.UV.xyz - ViewPosition) / FarPlane, Alpha, 1.0, 1.0);
};