#include "std/layouts/element.hlsl"
#include "std/channels/stream.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"

cbuffer RenderConstant : register(b3)
{
	matrix FaceView[6];
};

VOutputCubic Make(VOutputLinear V, uint Face)
{
	V.Position = mul(V.Position, FaceView[Face]);
	V.Position = mul(V.Position, vb_Proj);

	VOutputCubic Result = (VOutputCubic)0;
	Result.Position = V.Position;
	Result.UV = V.UV;
	Result.Alpha = V.Alpha;
	Result.RenderTarget = Face;

	return Result;
}

[maxvertexcount(6)]
void gs_main(point VOutputLinear V[1], inout PointStream<VOutputCubic> Stream)
{
	VOutputLinear Next = V[0];
	[unroll] for (uint Face = 0; Face < 6; Face++)
	{
		Stream.Append(Make(Next, Face));
		Stream.RestartStrip();
	}
}

VOutputLinear vs_main(VInput V)
{
	Element Base = Elements[V.Position];	
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(Base.Position, 1), ob_World);
	Result.Alpha = Base.Color.w;

	return Result;
}

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (1.0 - V.Alpha) * (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_Mid].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return length(V.UV.xyz - vb_Position) / vb_Far;
};