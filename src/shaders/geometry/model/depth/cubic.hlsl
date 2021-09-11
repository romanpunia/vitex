#include "std/layouts/vertex.hlsl"
#include "std/channels/depth.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/buffers/cubic.hlsl"

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
			Result.Position = mul(Next.Position, cb_ViewProjection[Face]);
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
	Result.Position = Result.UV = mul(float4(V.Position, 1.0), ob_World);
	Result.TexCoord = V.TexCoord * ob_TexCoord.xy;

	return Result;
}

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_Mid].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return length(V.UV.xyz - vb_Position) / vb_Far;
};