#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"

cbuffer RenderConstant : register(b3)
{
	float2 Texel;
    float FrameId;
	float Padding;
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float GetAnimatedInterlavedGradientNoise(float2 TexCoord)
{
    uint Frame = uint(FrameId);
    float2 UV = TexCoord * Texel;
    if ((Frame & 2) != 0)
        UV = float2(-UV.y, UV.x);
        
    if ((Frame & 1) != 0)
        UV.x = -UV.x;
    
    return frac(UV.x * 0.7548776662 + UV.y * 0.56984029 + float(Frame) * 0.41421356);
}
float3 GetPerpendicularVector(float3 Value)
{
    float3 Axis = abs(Value);
    if (Axis.x < Axis.y)
        return Axis.x < Axis.z ? float3(0, -Value.z, Value.y) : float3(-Value.y, Value.x, 0);
    
    return Axis.y < Axis.z ? float3(Value.z, 0, -Value.x) : float3(-Value.y, Value.x, 0);
}
float3 GetCosHemisphereSample(float Random1, float Random2, float3 Normal)
{
	float2 Direction = float2(Random1, Random2);
	float3 Bitangent = GetPerpendicularVector(Normal);
	float3 Tangent = cross(Bitangent, Normal);
	float Radius = sqrt(Direction.x);
	float PHI = 2.0f * 3.14159265f * Direction.y;

	return Tangent * (Radius * cos(PHI).x) + Bitangent * (Radius * sin(PHI)) + Normal.xyz * sqrt(max(0.0, 1.0f - Direction.x));
}

float4 ps_main(VOutput V) : SV_TARGET0
{
    float3 Normal = GetNormal(V.TexCoord.xy);
    float Noise = GetAnimatedInterlavedGradientNoise(V.TexCoord.xy);
    float3 StochasticNormal = GetCosHemisphereSample(Noise, Noise, Normal);
    return normalize(float4(StochasticNormal, 1.0));
};