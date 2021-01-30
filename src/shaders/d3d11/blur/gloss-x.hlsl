#include "std/layouts/shape"
#include "std/channels/effect"
#include "std/core/sampler"
#include "std/core/random"
#include "std/core/material"

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
	Result.Position = float4(V.Position, 1.0);
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

    return float4(B / max(1.0, I), 1.0);
};