#include "standard/space-uv"

cbuffer RenderConstant : register(b3)
{
    float2 Texel;
    float IterationCount;
    float Intensity;
    float Threshold;
    float Scale;
    float Padding1;
    float Padding2;
}

float4 PS(VertexResult V) : SV_TARGET0
{
    float3 B = float3(0, 0, 0);
    Fragment Frag = GetFragment(V.TexCoord.xy);
    Material Mat;

	[loop] for (int x = -IterationCount; x < IterationCount; x++)
	{
		[loop] for (int y = -IterationCount; y < IterationCount; y++)
		{
			float2 Offset = float2(x, y) * Texel * Scale;
			Frag = GetFragment(V.TexCoord.xy + Offset);
			Mat = GetMaterial(Frag.Material);

			B += saturate(Frag.Diffuse - Threshold) * Intensity;
		}
	}

    B /= 4 * IterationCount * IterationCount;
	return float4(Frag.Diffuse + B, 1.0f);
};