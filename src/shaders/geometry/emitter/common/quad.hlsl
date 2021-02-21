#include "std/layouts/element"
#include "std/channels/stream"
#include "std/buffers/object"
#include "std/buffers/viewer"

cbuffer RenderConstant : register(b3)
{
	matrix FaceView[6];
};

VOutputCubic Make(VOutputLinear V, float2 Offset, float2 TexCoord2, uint i)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position = mul(V.Position, FaceView[i]);
	V.Position += float4(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos, 0, 0);
	V.Position = mul(V.Position, vb_Proj);

	VOutputCubic Result = (VOutputCubic)0;
	Result.Position = V.Position;
	Result.UV = V.UV;
    Result.Normal = V.Normal;
	Result.Rotation = V.Rotation;
	Result.Scale = V.Scale;
	Result.Alpha = V.Alpha;
	Result.RenderTarget = i;
	Result.TexCoord = TexCoord2 * ob_TexCoord.xy;

	return Result;
}

[maxvertexcount(24)]
void gs_main(point VOutputLinear V[1], inout TriangleStream<VOutputCubic> Stream)
{
	for (uint i = 0; i < 6; i++)
	{
		Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(0, 0), i));
		Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(0, -1), i));
		Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(1, 0), i));
		Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(1, -1), i));
		Stream.RestartStrip();
	}
}

VOutputLinear vs_main(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = Result.UV = mul(float4(Elements[V.Position].Position, 1), ob_World);
    Result.Normal = float3(0, 0, 1);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Scale = Elements[V.Position].Scale;
	Result.Alpha = Elements[V.Position].Color.w;
    
	return Result;
}