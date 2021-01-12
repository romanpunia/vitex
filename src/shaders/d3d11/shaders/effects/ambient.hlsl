#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/random"
#include "sdk/core/position"
#include "sdk/core/material"

cbuffer RenderConstant : register(b3)
{
	float Samples;
	float Intensity;
	float Scale;
	float Bias;
	float Radius;
	float Distance;
	float Fade;
    float Padding;
}

float GetFactor(float2 TexCoord, float3 Position, float3 Normal, float Power) 
{
    float3 D = GetPosition(TexCoord, GetDepth(TexCoord)) - Position; 
    float3 V = normalize(D); 
    float T = length(D) * Scale; 

    return max(0.0, dot(Normal, V) - Bias) * (1.0 / (1.0 + T)) * Power;
}

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = float4(V.Position, 1.0);
	Result.TexCoord = Result.Position;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    const float2 Disk[4] = { float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1) };
	float2 TexCoord = GetTexCoord(V.TexCoord);
	Fragment Frag = GetFragment(TexCoord);

    [branch] if (Frag.Depth >= 1.0)
        return float4(1.0, 1.0, 1.0, 1.0);

    Material Mat = GetMaterial(Frag.Material);
    float2 Random = RandomFloat2(TexCoord);
	float Vision = saturate(pow(abs(distance(ViewPosition, Frag.Position) / Distance), Fade));
    float Power = Intensity * GetOcclusion(Frag, Mat);
    float Size = (Radius + Mat.Radius) / Frag.Depth;
    float Factor = 0.0;
    
    [loop] for (int j = 0; j < Samples; ++j) 
    {
        float2 C1 = reflect(Disk[j], Random) * Size; 
        float2 C2 = float2(C1.x * 0.707 - C1.y * 0.707, C1.x * 0.707 + C1.y * 0.707); 

        Factor += GetFactor(TexCoord + C1 * 0.25, Frag.Position, Frag.Normal, Power); 
        Factor += GetFactor(TexCoord + C2 * 0.5, Frag.Position, Frag.Normal, Power);
        Factor += GetFactor(TexCoord + C1 * 0.75, Frag.Position, Frag.Normal, Power);
        Factor += GetFactor(TexCoord + C2, Frag.Position, Frag.Normal, Power);
    }

    Factor /= Samples * Samples; 
    return 1.0 - Factor * Vision;
};