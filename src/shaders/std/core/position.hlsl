#include "std/buffers/viewer.hlsl"

float2 GetTexCoord(float4 UV)
{
	return float2(0.5f + 0.5f * UV.x / UV.w, 0.5f - 0.5f * UV.y / UV.w);
}
float3 GetPosition(float2 TexCoord, float Depth)
{
	float4 Position = float4(TexCoord.x * 2.0 - 1.0, 1.0 - TexCoord.y * 2.0, Depth, 1.0);
	Position = mul(Position, vb_InvViewProj);

	return Position.xyz / Position.w;
}
float3 GetPositionUV(float3 Position)
{
	float4 Coord = mul(float4(Position, 1.0), vb_ViewProj);
	Coord.xy = float2(0.5, 0.5) + float2(0.5, -0.5) * Coord.xy / Coord.w;
	Coord.z /= Coord.w;
	
	return Coord.xyz;
}