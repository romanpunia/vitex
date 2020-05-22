#include "shape-vertex-in.hlsl"
#include "shape-vertex-out.hlsl"
#include "geometry.hlsl"
#include "random-float-2.hlsl"

cbuffer RenderConstant : register(b3)
{
	float ScanLineJitterDisplacement;
	float ScanLineJitterThreshold;
	float VerticalJumpAmount;
	float VerticalJumpTime;
	float ColorDriftAmount;
	float ColorDriftTime;
	float HorizontalShake;
	float ElapsedTime;
}

ShapeVertexOut VS(ShapeVertexIn Input)
{
	ShapeVertexOut Output = (ShapeVertexOut)0;
	Output.Position = Input.Position;
	Output.TexCoord.xy = Input.TexCoord;
	return Output;
}

float4 PS(ShapeVertexOut Input) : SV_TARGET0
{
	float Jitter = RandomFloat2(Input.TexCoord.y, ElapsedTime) * 2.0f - 1.0f;
	Jitter *= step(ScanLineJitterThreshold, abs(Jitter)) * ScanLineJitterDisplacement;
	float Jump = lerp(Input.TexCoord.y, frac(Input.TexCoord.y + VerticalJumpTime), VerticalJumpAmount);
	float Shake = (RandomFloat2(ElapsedTime, 2) - 0.5) * HorizontalShake;
	float Drift = sin(Jump + ColorDriftTime) * ColorDriftAmount;
	float4 Alpha = Input1.Sample(State, frac(float2(Input.TexCoord.x + Jitter + Shake, Jump)));
	float4 Beta = Input1.Sample(State, frac(float2(Input.TexCoord.x + Jitter + Shake + Drift, Jump)));

	return float4(Alpha.r, Beta.g, Alpha.b, 1);
};