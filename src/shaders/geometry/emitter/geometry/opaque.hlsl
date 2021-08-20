#include "std/layouts/element.hlsl"
#include "std/channels/stream.hlsl"
#include "std/channels/gbuffer.hlsl"
#include "std/buffers/object.hlsl"

VOutputOpaque Make(VOutputOpaque V, float2 Offset, float2 TexCoord2)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, ob_World);
	V.TexCoord = TexCoord2 * ob_TexCoord.xy;
    V.UV = V.Position;
	return V;
}

[maxvertexcount(4)]
void gs_main(point VOutputOpaque V[1], inout TriangleStream<VOutputOpaque> Stream)
{
	Stream.Append(Make(V[0], float2(-1, -1) * V[0].Scale, float2(0, 0)));
	Stream.Append(Make(V[0], float2(-1, 1) * V[0].Scale, float2(0, -1)));
	Stream.Append(Make(V[0], float2(1, -1) * V[0].Scale, float2(1, 0)));
	Stream.Append(Make(V[0], float2(1, 1) * V[0].Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VOutputOpaque vs_main(VInput V)
{
	VOutputOpaque Result = (VOutputOpaque)0;
	Result.Position = mul(float4(Elements[V.Position].Position, 1), ob_WorldViewProj);
	Result.Rotation = Elements[V.Position].Rotation;
	Result.Color = Elements[V.Position].Color;
	Result.Scale = Elements[V.Position].Scale;
    Result.UV = Result.Position;
    Result.Normal = float3(0, 0, 1);
    Result.Tangent = float3(1, 0, 0);
    Result.Bitangent = float3(0, -1, 0);

	return Result;
}

GBuffer ps_main(VOutputOpaque V)
{
	float4 Color = float4(Materials[ob_Mid].Diffuse * V.Color.xyz, V.Color.w);
	[branch] if (ob_Diffuse > 0)
		Color *= GetDiffuse(V.TexCoord);

	float3 Normal = V.Normal;
	[branch] if (ob_Normal > 0)
        Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);

    return Compose(V.TexCoord, Color, Normal, V.UV.z / V.UV.w, ob_Mid);
};