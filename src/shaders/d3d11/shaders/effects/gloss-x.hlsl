#include "sdk/layouts/shape"
#include "sdk/channels/effect"
#include "sdk/core/sampler"
#include "sdk/core/random"
#include "sdk/core/material"

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Blur;
}

Texture2D Image : register(t5);

VOutput VS(VInput V)
{
	VOutput Result = (VOutput)0;
	Result.Position = V.Position;
	Result.TexCoord.xy = V.TexCoord;

	return Result;
}

float4 PS(VOutput V) : SV_TARGET0
{
    Fragment Frag = GetFragment(V.TexCoord.xy);
    Material Mat = GetMaterial(Frag.Material);
    float3 N = GetNormal(V.TexCoord.xy);
    float3 B = float3(0, 0, 0);
    float R = GetRoughnessMip(Frag, Mat, 1.0);
    float G = Samples * R;
    float I = 0.0;

	[loop] for (int i = 0; i < G; i++)
	{
        float2 T = V.TexCoord.xy + float2(FiboDisk[i].x, 0) * Texel * Blur * R;
        [branch] if (dot(GetNormal(T), N) < 0.0)
            continue;

        B += GetSampleLevel(Image, T, 0).xyz; I++;
	}

    [branch] if (I <= 0.0)
        return float4(0.0, 0.0, 0.0, 1.0);
    
    return float4(B / I, 1.0);
};