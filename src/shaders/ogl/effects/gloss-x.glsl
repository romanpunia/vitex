#include "standard/space-uv"

Texture2D Image : register(t5);

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float Samples;
    float Blur;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    Fragment Frag = GetFragment(V.TexCoord.xy);
    Material Mat = GetMaterial(Frag.Material);
    float3 N = GetNormal(V.TexCoord.xy);
    float3 B = float3(0, 0, 0);
    float R = GetRoughnessLevel(Frag, Mat, 1.0);
    float G = Samples * R;
    float I = 0.0;

	[loop] for (float x = -G; x < G; x++)
	{
        float2 T = V.TexCoord.xy + float2(x, 0) * Texel * Blur * R;
        [branch] if (dot(GetNormal(T), N) < 0.0)
            continue;

        B += GetSampleLevel(Image, T, 0).xyz; I++;
	}

    [branch] if (I <= 0.0)
        return float4(0.0, 0.0, 0.0, 1.0);
    
    return float4(B / I, 1.0);
};