#include "std/layouts/shape.hlsl"
#include "std/channels/effect.hlsl"

cbuffer RenderConstant : register(b3)
{
	float2 Padding;
	float Grayscale;
	float ACES;
	float Filmic;
	float Lottes;
	float Reinhard;
	float Reinhard2;
	float Unreal;
	float Uchimura;
	float UBrightness;
	float UContrast;
	float UStart;
	float ULength;
	float UBlack;
	float UPedestal;
	float Exposure;
	float EIntensity;
	float EGamma;
	float Adaptation;
	float AGray;
	float AWhite;
	float ABlack;
	float ASpeed;
}

Texture2D LUT : register(t5);

float3 GetGrayscale(float3 Color)
{
	return (Color.x + Color.y + Color.z) / 3.0;
}
float3 GetACES(float3 Color)
{
	const float A = 2.51;
	const float B = 0.03;
	const float C = 2.43;
	const float D = 0.59;
	const float E = 0.14;
	return clamp((Color * (A * Color + B)) / (Color * (C * Color + D) + E), 0.0, 1.0);
}
float3 GetFilmic(float3 Color)
{
	float3 X = max(0.0, Color - 0.004);
	return pow(abs((X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06)), 2.2);
}
float3 GetLottes(float3 Color)
{
	const float3 A = 1.6;
	const float3 D = 0.977;
	const float3 HDR = 8.0;
	const float3 In = 0.18;
	const float3 Out = 0.267;
	const float3 B = (-pow(abs(In), A) + pow(abs(HDR), A) * Out) / ((pow(abs(HDR), A * D) - pow(abs(In), A * D)) * Out);
	const float3 C = (pow(abs(HDR), A * D) * pow(abs(In), A) - pow(abs(HDR), A) * pow(abs(In), A * D) * Out) / ((pow(abs(HDR), A * D) - pow(abs(In), A * D)) * Out);

	return pow(abs(Color), A) / (pow(abs(Color), A * D) * B + C);
}
float3 GetReinhard(float3 Color)
{
	return Color / (1.0 + Color);
}
float3 GetReinhard2(float3 Color)
{
	const float White = 4.0 * 4.0;
	return (Color * (1.0 + Color / White)) / (1.0 + Color);
}
float3 GetUnreal(float3 Color)
{
	return Color / (Color + 0.155) * 1.019;
}
float3 GetUchimura(float3 Color)
{
	float K0 = ((UBrightness - UStart) * ULength) / UContrast;
	float L0 = UStart - UStart / UContrast;
	float L1 = UStart + (1.0 - UStart) / UContrast;
	float S0 = UStart + K0;
	float S1 = UStart + UContrast * K0;
	float C2 = (UContrast * UBrightness) / (UBrightness - S1);
	float CP = -C2 / UBrightness;
	float3 W0 = 1.0 - smoothstep(0.0, UStart, Color);
	float3 W2 = step(UStart + K0, Color);
	float3 W1 = 1.0 - W0 - W2;
	float3 T = UStart * pow(abs(Color / UStart), UBlack) + UPedestal;
	float3 S = UBrightness - (UBrightness - S1) * exp(CP * (Color - S0));
	float3 L = UStart + UContrast * (Color - UStart);

	return T * W0 + L * W1 + S * W2;
}
float3 GetExposure(float3 Color)
{
	float3 Result = 1.0 - exp(-Color * EIntensity);
	return pow(abs(Result), 1.0 / EGamma);
}
float3 GetAdaptation(float3 Color)
{
	float Luminance = LUT.Load(int3(0, 0, 0)).r;
	float Avg = saturate((Color.x + Color.y + Color.z) / 3.0);
	float Exp = AGray / clamp(Luminance, ABlack, AWhite);

	return Color * Exp;
}

VOutput vs_main(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 ps_main(VOutput V) : SV_TARGET0
{
	float3 Result = GetDiffuse(V.TexCoord.xy, 0).xyz;
	Result = lerp(Result, GetAdaptation(Result), Adaptation);
	Result = lerp(Result, GetGrayscale(Result), Grayscale);
	Result = lerp(Result, GetACES(Result), ACES);
	Result = lerp(Result, GetFilmic(Result), Filmic);
	Result = lerp(Result, GetLottes(Result), Lottes);
	Result = lerp(Result, GetReinhard(Result), Reinhard);
	Result = lerp(Result, GetReinhard2(Result), Reinhard2);
	Result = lerp(Result, GetUnreal(Result), Unreal);
	Result = lerp(Result, GetUchimura(Result), Uchimura);
	Result = lerp(Result, GetExposure(Result), Exposure);
	
	return float4(Result, 1.0);
};