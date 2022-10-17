#include "std/layouts/interface.hlsl"
#include "std/channels/immediate.hlsl"
#include "std/buffers/object.hlsl"

cbuffer RenderConstant : register(b3)
{
	float4 Color;
	float4 Radius;
	float2 Size;
	float2 Position;
	float2 Offset;
	float Softness;
	float Padding;
}

float Rectangle(float2 Position, float2 Size, float4 Radius)
{
	float2 T = step(Position, float2(0.0, 0.0));
	float R = lerp(lerp(Radius.y, Radius.z, T.y), lerp(Radius.x, Radius.w, T.y), T.x);
	return length(max(abs(Position) + float2(R, R) - Size, 0.0)) - R;
}

VOutput vs_main(VInput V)
{
	VOutput Result;
	Result.Position = mul(float4(V.Position.xy, 0.0, 1.0), ob_Transform);
	Result.UV = mul(Result.Position, ob_World);
	Result.Color = float4(0.0, 0.0, 0.0, 0.0);
	Result.TexCoord = float2(0.0, 0.0);

	return Result;
};

float4 ps_main(VOutput V) : SV_Target
{
	float2 Center = V.UV.xy - Size / 2.0;
	float Distance1 = Rectangle(Center, Size / 2.0 - 1, Radius);
	float Alpha1 = saturate(1.0 - smoothstep(0.0, 2.0, Distance1)); 
	float Distance2 = Rectangle(Center + Offset, Size / 2.0, Radius);
	float Alpha2 = saturate(1.0 - smoothstep(-Softness, Softness, Distance2));
	float4 Result = lerp(float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0) - Alpha1, Alpha1);
	
	return lerp(Result, Color, Alpha2 - Alpha1);
};