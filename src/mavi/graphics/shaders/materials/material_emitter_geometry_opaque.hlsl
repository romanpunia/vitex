#include "internal/layouts_element.hlsl"
#include "internal/channels_stream.hlsl"
#include "internal/channels_gbuffer.hlsl"
#include "internal/buffers_object.hlsl"

VOutputOpaque Make(VOutputOpaque V, float2 Offset, float2 Coord)
{
	float Sin = sin(V.Rotation), Cos = cos(V.Rotation);
	V.Position.xy += float2(Offset.x * Cos - Offset.y * Sin, Offset.x * Sin + Offset.y * Cos);
	V.Position = mul(V.Position, ob_World);
	V.TexCoord = Coord;
	V.UV = V.Position;
	
	return V;
}

[maxvertexcount(4)]
void gs_main(point VOutputOpaque V[1], inout TriangleStream<VOutputOpaque> Stream)
{
	VOutputOpaque Next = V[0];
	Stream.Append(Make(Next, float2(-1, -1) * Next.Scale, float2(0, 0)));
	Stream.Append(Make(Next, float2(-1, 1) * Next.Scale, float2(0, -1)));
	Stream.Append(Make(Next, float2(1, -1) * Next.Scale, float2(1, 0)));
	Stream.Append(Make(Next, float2(1, 1) * Next.Scale, float2(1, -1)));
	Stream.RestartStrip();
}

VOutputOpaque vs_main(VInput V)
{
	Element Base = Elements[V.Position];
	VOutputOpaque Result = (VOutputOpaque)0;
	Result.Position = mul(float4(Base.Position, 1), ob_Transform);
	Result.Rotation = Base.Rotation;
	Result.Color = Base.Color;
	Result.Scale = Base.Scale;
	Result.Normal = ob_TexCoord.xyz;
	Result.Tangent = float3(1, 0, 0);
	Result.Bitangent = float3(0, -1, 0);
	Result.UV = Result.Position;

	return Result;
}

GBuffer ps_main(VOutputOpaque V)
{
	float4 Color = float4(Materials[ob_MaterialId].Diffuse * V.Color.xyz, V.Color.w);
	[branch] if (ob_Diffuse > 0)
	{
		Color *= GetDiffuse(V.TexCoord);
		if (Color.w < 0.001)
			discard;
	}

	float3 Normal = V.Normal;
	[branch] if (ob_Normal > 0)
		Normal = GetNormal(V.TexCoord, V.Normal, V.Tangent, V.Bitangent);

	return Compose(V.TexCoord, Color, Normal, V.UV.z / V.UV.w, ob_MaterialId);
};