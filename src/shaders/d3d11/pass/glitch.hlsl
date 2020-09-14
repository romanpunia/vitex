#include "standard/space-uv"
#include "standard/random-float"

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

float4 PS(VertexResult V) : SV_TARGET0
{
	float Jitter = RandomFloatXY(V.TexCoord.y, ElapsedTime) * 2.0 - 1.0;
	Jitter *= step(ScanLineJitterThreshold, abs(Jitter)) * ScanLineJitterDisplacement;
	float Jump = lerp(V.TexCoord.y, frac(V.TexCoord.y + VerticalJumpTime), VerticalJumpAmount);
	float Shake = (RandomFloatXY(ElapsedTime, 2) - 0.5) * HorizontalShake;
	float Drift = sin(Jump + ColorDriftTime) * ColorDriftAmount;
	float4 Alpha = GetDiffuse(frac(float2(V.TexCoord.x + Jitter + Shake, Jump)));
	float4 Beta = GetDiffuse(frac(float2(V.TexCoord.x + Jitter + Shake + Drift, Jump)));

	return float4(Alpha.r, Beta.g, Alpha.b, 1);
};