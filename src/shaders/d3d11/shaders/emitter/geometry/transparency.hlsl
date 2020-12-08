#include "sdk/layouts/element"
#include "sdk/channels/stream"
#include "sdk/channels/gbuffer"
#include "sdk/buffers/object"

VOutput Make(VOutput V, float2 Offset, float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, World);
	V.TexCoord = TexCoord2;
	return V;
}

[maxvertexcount(4)]
void GS(point VOutput V[1], inout TriangleStream<VOutput> Stream)
{
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), WorldViewProjection);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Color = Elements[V.Position].Color;
	Result.Scale = Elements[V.Position].Scale;
    
	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    float4 Color = float4(Diffuse * V.Color.xyz, V.Color.w);
	[branch] if (HasDiffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	return Color;
};