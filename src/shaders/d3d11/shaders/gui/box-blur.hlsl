#include "sdk/layouts/gui"
#include "sdk/channels/immediate"
#include "sdk/buffers/object"

cbuffer RenderConstant : register(b3)
{
    float4 Radius;
    float2 Texel;
    float2 Size;
    float2 Position;
    float Softness;
    float Alpha;
}

float Rectangle(float2 Position, float2 Size, float4 Radius)
{
    float2 T = step(Position, float2(0.0, 0.0));
    float R = lerp(lerp(Radius.y, Radius.z, T.y), lerp(Radius.x, Radius.w, T.y), T.x);
    return length(max(abs(Position) + float2(R, R) - Size, 0.0)) - R;
}

VOutput VS(VInput V)
{
	VOutput Result;
	Result.Position = mul(float4(V.Position.xy, 0.0, 1.0), WorldViewProjection);
    Result.UV = mul(Result.Position, World);
    Result.Color = float4(0.0, 0.0, 0.0, 0.0);
    Result.TexCoord = V.TexCoord;

	return Result;
};

float4 PS(VOutput V) : SV_Target
{
    float Distance = Rectangle(V.UV.xy - Size / 2.0, Size / 2.0 - 1, Radius);
    [branch] if (saturate(1.0 - smoothstep(0.0, 2.0, Distance)) < 0.25)
        return float4(0, 0, 0, 0);

    const float Directions = 16.0;
    const float Quality = 3.0;
    float2 Cover = Softness;
    float3 Color = DiffuseMap.Load(int3(V.Position.xy, 0)).xyz;

    [unroll] for (float d = 0.0; d < 6.283185; d += 6.283185 / Directions)
    {
		[unroll] for (float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality)
			Color += DiffuseMap.Load(int3(V.Position.xy + float2(cos(d), sin(d)) * Cover * i, 0)).xyz;	
    }

    Color /= Quality * Directions;
    return float4(Color * Diffuse, Alpha);
};