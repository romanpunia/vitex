#include "std/layouts/skin.hlsl"
#include "std/channels/depth.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/buffers/cubic.hlsl"
#include "std/buffers/animation.hlsl"

[maxvertexcount(18)]
void gs_main(triangle VOutputLinear V[3], inout TriangleStream<VOutputCubic> Stream)
{
	VOutputCubic Result = (VOutputCubic)0;
	for (Result.RenderTarget = 0; Result.RenderTarget < 6; Result.RenderTarget++)
	{
		Result.Position = Result.UV = mul(V[0].Position, cb_ViewProjection[Result.RenderTarget]);
		Result.UV = V[0].UV;
		Result.TexCoord = V[0].TexCoord;
		Result.Normal = V[0].Normal;
		Stream.Append(Result);

		Result.Position = Result.UV = mul(V[1].Position, cb_ViewProjection[Result.RenderTarget]);
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
	Result.Normal = normalize(mul(V.Normal, (float3x3)ob_World));
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	[branch] if (ab_Animated > 0)
	{
		matrix Offset =
			mul(ab_Offsets[(int)V.Index.x], V.Bias.x) +
			mul(ab_Offsets[(int)V.Index.y], V.Bias.y) +
			mul(ab_Offsets[(int)V.Index.z], V.Bias.z) +
			mul(ab_Offsets[(int)V.Index.w], V.Bias.w);

		Result.Position = Result.UV = mul(mul(float4(V.Position, 1.0), Offset), ob_World);
	}
	else
		Result.Position = Result.UV = mul(float4(V.Position, 1.0), ob_World);

	return Result;
}