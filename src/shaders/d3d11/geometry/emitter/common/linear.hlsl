#include "std/layouts/element"
#include "std/channels/stream"
#include "std/buffers/object"

VOutputLinear Make(VOutputLinear V, float2 Offset, float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, World);
	V.TexCoord = TexCoord2;
    V.Normal = float3(0, 0, 1);
    
	return V;
}

[maxvertexcount(4)]
void GS(point VOutputLinear V[1], inout TriangleStream<VOutputLinear> Stream)
{
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VOutputLinear VS(VInput V)
{
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), WorldViewProjection);
    Result.Normal = float3(0, 0, 1);
	Result.UV = mul(Result.Position, World);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Scale = Elements[V.Position].Scale;
	Result.Alpha = Elements[V.Position].Color.w;

	return Result;
}