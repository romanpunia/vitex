#include "std/layouts/vertex.hlsl"
#include "std/channels/depth.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/buffers/cubic.hlsl"

[maxvertexcount(18)]
void gs_main(triangle VOutputLinear V[3], inout TriangleStream<VOutputCubic> Stream)
{
	VOutputCubic Result = (VOutputCubic)0;
	for (Result.RenderTarget = 0; Result.RenderTarget < 6; Result.RenderTarget++)
	{
		Result.Position = mul(V[0].Position, cb_ViewProjection[Result.RenderTarget]);
		Result.UV = V[0].UV;
		Result.TexCoord = V[0].TexCoord;
		Result.Normal = V[0].Normal;
		Stream.Append(Result);

		Result.Position = mul(V[1].Position, cb_ViewProjection[Result.RenderTarget]);
		Result.UV = V[1].UV;
		Result.TexCoord = V[1].TexCoord;
		Result.Normal = V[1].Normal;
		Stream.Append(Result);

		Result.Position = mul(V[2].Position, cb_ViewProjection[Result.RenderTarget]);
		Result.UV = V[2].UV;
		Result.TexCoord = V[2].TexCoord;
		Result.Normal = V[2].Normal;
		Stream.Append(Result);

		Stream.RestartStrip();
	}
}

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), ob_World);
	Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	return Result;
}