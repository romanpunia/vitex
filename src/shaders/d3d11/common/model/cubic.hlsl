#include "sdk/layouts/vertex"
#include "sdk/channels/gflux"
#include "sdk/buffers/object"
#include "sdk/buffers/viewer"
#include "sdk/buffers/cubic"

[maxvertexcount(18)]
void GS(triangle VOutputLinear V[3], inout TriangleStream<VOutputCubic> Stream)
{
	VOutputCubic Result = (VOutputCubic)0;
	for (Result.RenderTarget = 0; Result.RenderTarget < 6; Result.RenderTarget++)
	{
		Result.Position = mul(V[0].Position, FaceViewProjection[Result.RenderTarget]);
		Result.UV = V[0].UV;
		Result.TexCoord = V[0].TexCoord;
        Result.Normal = V[0].Normal;
		Stream.Append(Result);

		Result.Position = mul(V[1].Position, FaceViewProjection[Result.RenderTarget]);
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
	Result.Position = Result.UV = mul(V.Position, World);
	Result.Normal = normalize(mul(V.Normal, (float3x3)World));
	Result.TexCoord = V.TexCoord * TexCoord;

	return Result;
}