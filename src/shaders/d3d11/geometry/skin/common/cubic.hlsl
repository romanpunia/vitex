#include "std/layouts/skin"
#include "std/channels/depth"
#include "std/buffers/object"
#include "std/buffers/viewer"
#include "std/buffers/cubic"
#include "std/buffers/animation"

[maxvertexcount(18)]
void GS(triangle VOutputLinear V[3], inout TriangleStream<VOutputCubic> Stream)
{
	VOutputCubic Result = (VOutputCubic)0;
	for (Result.RenderTarget = 0; Result.RenderTarget < 6; Result.RenderTarget++)
	{
		Result.Position = Result.UV = mul(V[0].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[0].UV;
		Result.TexCoord = V[0].TexCoord;
        Result.Normal = V[0].Normal;
		Stream.Append(Result);

		Result.Position = Result.UV = mul(V[1].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[1].UV;
		Result.TexCoord = V[1].TexCoord;
        Result.Normal = V[1].Normal;
		Stream.Append(Result);

		Result.Position = mul(V[2].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[2].UV;
		Result.TexCoord = V[2].TexCoord;
        Result.Normal = V[2].Normal;
		Stream.Append(Result);

		Stream.RestartStrip();
	}
}

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

		Result.Position = Result.UV = mul(mul(float4(V.Position, 1.0), Offset), World);
	}
	else
		Result.Position = Result.UV = mul(float4(V.Position, 1.0), World);

	return Result;
}