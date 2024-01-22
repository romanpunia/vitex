#include "std/layouts/element.hlsl"
#include "std/channels/stream.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"

cbuffer RenderConstant : register(b3)
{
	matrix FaceView[6];
};

VOutputCubic Make(VOutputLinear V, float2 Offset, float2 Coord, uint Face)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position = mul(V.Position, FaceView[Face]);
	V.Position += float4(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos, 0, 0);
	V.Position = mul(V.Position, vb_Proj);

	VOutputCubic Result = (VOutputCubic)0;
	Result.Position = V.Position;
	Result.UV = V.UV;
	Result.Rotation = V.Rotation;
	Result.Scale = V.Scale;
	Result.Alpha = V.Alpha;
	Result.RenderTarget = Face;
	Result.TexCoord = Coord;

	return Result;
}

[maxvertexcount(24)]
void gs_main(point VOutputLinear V[1], inout TriangleStream<VOutputCubic> Stream)
{
	VOutputLinear Next = V[0];
	[unroll] for (uint Face = 0; Face < 6; Face++)
	{
		Stream.Append(Make(Next, float2(-1, -1) * Next.Scale, float2(0, 0), Face));
		Stream.Append(Make(Next, float2(-1, 1) * Next.Scale, float2(0, -1), Face));
		Stream.Append(Make(Next, float2(1, -1) * Next.Scale, float2(1, 0), Face));
		Stream.Append(Make(Next, float2(1, 1) * Next.Scale, float2(1, -1), Face));
		Stream.RestartStrip();
	}
}

VOutputLinear vs_main(VInput V)
{
	Element Base = Elements[V.Position];
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(Base.Position, 1), ob_World);
	Result.Rotation = Base.Rotation;
	Result.Scale = Base.Scale;
	Result.Alpha = Base.Color.w;
	
	return Result;
}

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (1.0 - V.Alpha) * (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_MaterialId].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return length(V.UV.xyz - vb_Position) / vb_Far;
};