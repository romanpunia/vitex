#include "std/layouts/element.hlsl"
#include "std/channels/stream.hlsl"
#include "std/channels/gbuffer.hlsl"
#include "std/buffers/object.hlsl"

VOutput Make(VOutput V, float2 Offset, float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, ob_World);
	V.TexCoord = TexCoord2 * ob_TexCoord.xy;
	return V;
}

[maxvertexcount(4)]
void gs_main(point VOutput V[1], inout TriangleStream<VOutput> Stream)
{
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), ob_WorldViewProj);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Color = Elements[V.Position].Color;
	Result.Scale = Elements[V.Position].Scale;
    
	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
    float4 Color = float4(Materials[ob_Mid].Diffuse * V.Color.xyz, V.Color.w);
	[branch] if (ob_Diffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	return Color;
};