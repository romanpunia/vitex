#include "std/layouts/shape.hlsl"
#include "std/channels/geometry.hlsl"
#include "std/buffers/object.hlsl"
#include "std/buffers/viewer.hlsl"
#include "std/core/position.hlsl"

Texture2D LDepthBuffer : register(t8);

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = mul(float4(V.Position, 1.0), ob_Transform);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	float2 TexCoord = GetTexCoord(V.TexCoord);
	float Depth = LDepthBuffer.SampleLevel(Sampler, TexCoord, 0).x;
	[branch] if (Depth >= 1.0)
		discard;
	
	float4 Position = mul(float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, Depth, 1.0), ob_World);
	clip(0.5 - abs(Position.xyz));

	float4 Color = float4(Materials[ob_MaterialId].Diffuse, 1.0);
	[branch] if (ob_Diffuse > 0)
	{
		Color *= GetDiffuse(TexCoord * ob_TexCoord.xy);
		if (Color.w < 0.001)
			discard;
	}

	return Color;
};