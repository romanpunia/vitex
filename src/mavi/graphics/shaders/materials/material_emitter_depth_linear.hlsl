#include "internal/layouts_element.hlsl"
#include "internal/channels_stream.hlsl"
#include "internal/buffers_object.hlsl"

VOutputLinear Make(VOutputLinear V, float2 Offset, float2 Coord)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, ob_World);
	V.TexCoord = Coord;
	V.UV = V.Position;
	
	return V;
}

[maxvertexcount(4)]
void gs_main(point VOutputLinear V[1], inout TriangleStream<VOutputLinear> Stream)
{
	VOutputLinear Next = V[0];
	Stream.Append(Make(Next, float2(-1, -1) * Next.Scale, float2(0, 0)));
	Stream.Append(Make(Next, float2(-1, 1) * Next.Scale, float2(0, -1)));
	Stream.Append(Make(Next, float2(1, -1) * Next.Scale, float2(1, 0)));
	Stream.Append(Make(Next, float2(1, 1) * Next.Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VOutputLinear vs_main(VInput V)
{
	Element Base = Elements[V.Position];
	VOutputLinear Result = (VOutputLinear)0;
	Result.Position = mul(float4(Base.Position, 1), ob_Transform);
	Result.Rotation = Base.Rotation;
	Result.Scale = Base.Scale;
	Result.Alpha = Base.Color.w;
	Result.UV = Result.Position;

	return Result;
}

float ps_main(VOutputLinear V) : SV_DEPTH
{
	float Threshold = (1.0 - V.Alpha) * (ob_Diffuse ? 1.0 - GetDiffuse(V.TexCoord).w : 1.0) * Materials[ob_MaterialId].Transparency;
	[branch] if (Threshold > 0.5)
		discard;
	
	return V.UV.z / V.UV.w;
};