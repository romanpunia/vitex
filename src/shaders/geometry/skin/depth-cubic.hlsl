#include "std/layouts/skin.hlsl"
#include "std/channels/depth.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/buffers/cubic.hlsl"
#include "std/buffers/animation.hlsl"

[maxvertexcount(18)]
void gs_main(triangle VOutputLinear V[3], inout TriangleStream<VOutputCubic> Stream)
{
	[unroll] for (uint Face = 0; Face < 6; Face++)
	{
		VOutputCubic Result = (VOutputCubic)0;
		Result.RenderTarget = Face;

		[unroll] for (uint Vertex = 0; Vertex < 3; Vertex++)
		{
			VOutputLinear Next = V[Vertex];	
			Result.Position = Result.UV = mul(Next.Position, cb_ViewProjection[Face]);
			Result.UV = Next.UV;
			Result.TexCoord = Next.TexCoord;
			Stream.Append(Result);
		}

		Stream.RestartStrip();
	}
}

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
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

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_MaterialId].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return length(V.UV.xyz - vb_Position) / vb_Far;
};