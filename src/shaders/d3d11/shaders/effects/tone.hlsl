#include "sdk/layouts/shape"
#include "sdk/channels/effect"

cbuffer RenderConstant : register(b3)
{
	float3 BlindVisionR;
	float VignetteAmount;
	float3 BlindVisionG;
	float VignetteCurve;
	float3 BlindVisionB;
	float VignetteRadius;
	float3 VignetteColor;
	float LinearIntensity;
	float3 ColorGamma;
	float GammaIntensity;
	float3 DesaturationGamma;
	float DesaturationIntensity;
    float ToneIntensity;
    float AcesIntensity;
    float AcesA;
    float AcesB;
    float AcesC;
    float AcesD;
    float AcesE;
    float Padding;
}

float3 GetToneMapping(float3 Color)
{
	return float3(dot(Color, BlindVisionR.xyz), dot(Color, BlindVisionG.xyz), dot(Color, BlindVisionB.xyz)) * ColorGamma;
}
float3 GetAcesFilm(float3 Color)
{
    return clamp((Color * (AcesA * Color + AcesB)) / (Color * (AcesC * Color + AcesD) + AcesE), 0.0, 1.0);
}
float3 GetDesaturation(float3 Pixel)
{
	float3 Color = Pixel;
	float Intensity = DesaturationGamma.x * Color.r + DesaturationGamma.y * Color.b + DesaturationGamma.z * Color.g;
	Intensity *= DesaturationIntensity;

	Color.r = Intensity + Color.r * (1 - DesaturationIntensity);
	Color.g = Intensity + Color.g * (1 - DesaturationIntensity);
	Color.b = Intensity + Color.b * (1 - DesaturationIntensity);

	return Color;
}

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
	float3 Color = GetDiffuse(V.TexCoord.xy, 0).xyz;
    Color = pow(abs(Color), 1.0 / GammaIntensity) * LinearIntensity + Color * (1.0 - LinearIntensity);
    
	float Vignette = saturate(distance((V.TexCoord.xy + 0.5) / VignetteRadius - 0.5, -float2(0.5, 0.5)));
	Color = lerp(Color, VignetteColor.xyz, pow(Vignette, VignetteCurve) * VignetteAmount);
    Color = GetToneMapping(Color) * ToneIntensity + Color * (1.0 - ToneIntensity);
    Color = GetAcesFilm(Color) * AcesIntensity + Color * (1.0 - AcesIntensity);
    Color = GetDesaturation(Color);

	return float4(Color, 1);
};