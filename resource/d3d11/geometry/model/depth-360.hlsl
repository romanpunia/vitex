#include "renderer/vertex"
#include "renderer/buffer/object"
#include "renderer/buffer/cubic"
#include "workflow/rasterizer"

[maxvertexcount(18)]
void GS(triangle VertexResult90 V[3], inout TriangleStream<VertexResult360> Stream)
{
	VertexResult360 Result = (VertexResult360)0;
	for (Result.RenderTarget = 0; Result.RenderTarget < 6; Result.RenderTarget++)
	{
		Result.Position = mul(V[0].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[0].UV;
		Result.TexCoord = V[0].TexCoord;
		Stream.Append(Result);

		Result.Position = mul(V[1].Position, FaceViewProjection[Result.RenderTarget]);
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
	Result.Position = Result.UV = mul(V.Position, World);
	Result.TexCoord = V.TexCoord * TexCoord;

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

	return float4(length(V.UV.xyz - ViewPosition) / FarPlane, Alpha, 1.0, 1.0);
};